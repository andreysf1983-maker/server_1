using System.Diagnostics;
using System.IO;
using System.Text;

namespace LegionForge.Manager.Services;

/// <summary>
/// LEGIONFORGE :: обёртка над одним процессом сервера (mysqld / bnetserver /
/// worldserver). Умеет: старт, корректный стоп (командой в консоль ядра),
/// перехват stdout/stderr с доставкой строк в UI и авто-рестарт при падении.
/// </summary>
public sealed class ProcessHost : IDisposable
{
    public string Name { get; }
    public string ExecutablePath { get; }
    public string Arguments { get; }
    public string WorkingDirectory { get; }
    public bool AutoRestart { get; set; }
    public bool IsRunning => _process is { HasExited: false };

    public event Action<string, string>? LineReceived;   // (source, line)
    public event Action<string>? StateChanged;

    private Process? _process;
    private bool _stopping;

    public ProcessHost(string name, string executablePath, string arguments, string workingDirectory)
    {
        Name = name;
        ExecutablePath = executablePath;
        Arguments = arguments;
        WorkingDirectory = Directory.Exists(workingDirectory)
            ? workingDirectory
            : Path.GetDirectoryName(executablePath) ?? ".";
    }

    public void Start()
    {
        if (IsRunning) return;
        _stopping = false;

        var psi = new ProcessStartInfo
        {
            FileName = ExecutablePath,
            Arguments = Arguments,
            WorkingDirectory = WorkingDirectory,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            RedirectStandardInput = true,
            CreateNoWindow = true,
            StandardOutputEncoding = Encoding.UTF8,
            StandardErrorEncoding = Encoding.UTF8
        };

        _process = new Process { StartInfo = psi, EnableRaisingEvents = true };
        _process.OutputDataReceived += (_, e) => { if (e.Data is not null) LineReceived?.Invoke(Name, e.Data); };
        _process.ErrorDataReceived  += (_, e) => { if (e.Data is not null) LineReceived?.Invoke(Name, e.Data); };
        _process.Exited += OnExited;

        try
        {
            _process.Start();
            _process.BeginOutputReadLine();
            _process.BeginErrorReadLine();
            StateChanged?.Invoke("запущен");
            LineReceived?.Invoke(Name, $"[LEGIONFORGE] {Name} запущен (PID {_process.Id}).");
        }
        catch (Exception ex)
        {
            LineReceived?.Invoke(Name, $"[ОШИБКА] Не удалось запустить {Name}: {ex.Message}");
            StateChanged?.Invoke("ошибка запуска");
        }
    }

    /// <summary>Мягкая остановка: сначала команда ядру в stdin, затем kill.</summary>
    public void Stop(bool graceful = true)
    {
        if (_process is null || _process.HasExited) return;
        _stopping = true;

        if (graceful)
        {
            try
            {
                _process.StandardInput.WriteLine(Name.Contains("world", StringComparison.OrdinalIgnoreCase) ? "server shutdown 10" : "exit");
                _process.StandardInput.Flush();
                if (!_process.WaitForExit(12000)) _process.Kill(true);
            }
            catch { try { _process.Kill(true); } catch { } }
        }
        else { try { _process.Kill(true); } catch { } }

        StateChanged?.Invoke("остановлен");
    }

    /// <summary>Мягкий ребут ядра с таймером-оповещением в мир.</summary>
    public void SoftRestart(int seconds)
    {
        if (_process is null || _process.HasExited) return;
        try
        {
            _process.StandardInput.WriteLine($"server shutdown {seconds}");
            _process.StandardInput.Flush();
            LineReceived?.Invoke(Name, $"[LEGIONFORGE] Запланирован ребут ядра через {seconds} сек.");
        }
        catch (Exception ex) { LineReceived?.Invoke(Name, "[ОШИБКА] " + ex.Message); }
    }

    public void SendCommand(string command)
    {
        if (_process is null || _process.HasExited) return;
        try
        {
            _process.StandardInput.WriteLine(command);
            _process.StandardInput.Flush();
        }
        catch (Exception ex) { LineReceived?.Invoke(Name, "[ОШИБКА] " + ex.Message); }
    }

    private void OnExited(object? sender, EventArgs e)
    {
        StateChanged?.Invoke("остановлен");
        if (_stopping || !AutoRestart) return;

        LineReceived?.Invoke(Name, $"[ВНИМАНИЕ] {Name} упал. Авто-рестарт через 5 секунд ...");
        Task.Delay(5000).ContinueWith(_ =>
        {
            try { Start(); }
            catch (Exception ex) { LineReceived?.Invoke(Name, "[ОШИБКА] Авто-рестарт не удался: " + ex.Message); }
        });
    }

    public void Dispose()
    {
        try { _process?.Dispose(); } catch { }
    }
}
