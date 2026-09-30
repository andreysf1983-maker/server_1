using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;
using LegionForge.Manager.Services;

namespace LegionForge.Manager;

/// <summary>
/// LEGIONFORGE :: главное окно менеджера сервера.
/// Запускает mysqld, bnetserver и worldserver, выводит их логи во вкладки
/// с подсветкой (ошибки - красным, успех - зелёным), предоставляет кнопки
/// управления и поднимает worldserver при падении.
/// </summary>
public partial class MainWindow : Window
{
    private readonly string _root;
    private readonly string _bin;
    private readonly string _configs;
    private readonly string _logs;
    private readonly ProcessHost _mysql;
    private readonly ProcessHost _bnet;
    private readonly ProcessHost _world;
    private readonly GameDatabase _db = new("127.0.0.1", 3306, "root", "legionforge");
    private readonly DispatcherTimer _statsTimer;
    private readonly Dictionary<string, FlowDocument> _docs = new();

    public MainWindow()
    {
        InitializeComponent();

        // Корень платформы: ...\LEGIONFORGE\server\bin\LegionForge_Manager.exe -> ...\LEGIONFORGE
        _bin = AppContext.BaseDirectory;
        _root = Directory.GetParent(Directory.GetParent(_bin)!.FullName)!.FullName;
        _configs = Path.Combine(_root, "server", "configs");
        _logs = Path.Combine(_root, "server", "logs");
        Directory.CreateDirectory(_logs);

        _mysql = new ProcessHost("MySQL",
            Path.Combine(_root, "tools", "mysql", "bin", "mysqld.exe"),
            $"--defaults-file=\"{Path.Combine(_root, "tools", "mysql", "my.ini")}\" --console",
            Path.Combine(_root, "tools", "mysql"));

        _bnet = new ProcessHost("BNetServer",
            Path.Combine(_bin, "bnetserver.exe"),
            $"--config=\"{Path.Combine(_configs, "bnetserver.conf")}\"", _bin);

        _world = new ProcessHost("WorldServer",
            Path.Combine(_bin, "worldserver.exe"),
            $"--config=\"{Path.Combine(_configs, "worldserver.conf")}\"", _bin);

        Hook(_mysql, LogMySql, "MySQL");
        Hook(_bnet, LogBnet, "BNetServer");
        Hook(_world, LogConsole, "WorldServer");

        _docs["MySQL"] = LogMySql.Document!;
        _docs["BNetServer"] = LogBnet.Document!;
        _docs["WorldServer"] = LogConsole.Document!;
        _docs["World"] = LogWorld.Document!;

        _statsTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(5) };
        _statsTimer.Tick += (_, _) => RefreshStats();
        _statsTimer.Start();

        RefreshStats();
    }

    private void Hook(ProcessHost host, FlowDocumentScrollViewer view, string tabKey)
    {
        host.LineReceived += (source, line) => Dispatcher.BeginInvoke(() => AppendLine(tabKey, source, line));
        host.StateChanged += state => Dispatcher.BeginInvoke(() => SetStatus($"{host.Name}: {state}"));
        // Дублируем мировые логи worldserver в отдельную вкладку
        if (ReferenceEquals(view, LogConsole))
            host.LineReceived += (_, line) => Dispatcher.BeginInvoke(() => AppendLine("World", host.Name, line));
    }

    private static readonly Brush ErrorBrush = new SolidColorBrush(Color.FromRgb(0xFF, 0x5A, 0x5A));
    private static readonly Brush WarnBrush  = new SolidColorBrush(Color.FromRgb(0xF5, 0xA6, 0x23));
    private static readonly Brush OkBrush    = new SolidColorBrush(Color.FromRgb(0x3D, 0xDC, 0x84));
    private static readonly Brush InfoBrush  = new SolidColorBrush(Color.FromRgb(0xC9, 0xD6, 0xE8));

    private void AppendLine(string tabKey, string source, string line)
    {
        if (!_docs.TryGetValue(tabKey, out var doc)) return;

        var brush = InfoBrush;
        var lower = line.ToLowerInvariant();
        if (lower.Contains("error") || lower.Contains("ошибка") || lower.Contains("fatal") || lower.Contains("exception")) brush = ErrorBrush;
        else if (lower.Contains("warn") || lower.Contains("внимание")) brush = WarnBrush;
        else if (lower.Contains("success") || lower.Contains("готово") || lower.Contains("started") || lower.Contains("запущен") || lower.Contains("ok")) brush = OkBrush;

        var para = new Paragraph(new Run($"[{DateTime.Now:HH:mm:ss}] [{source}] {line}"))
        {
            Foreground = brush,
            FontFamily = new FontFamily("Consolas"),
            FontSize = 12,
            Margin = new Thickness(0, 0, 0, 1)
        };
        doc.Blocks.Add(para);
        while (doc.Blocks.Count > 2000) doc.Blocks.Remove(doc.Blocks.FirstBlock!);

        // Параллельно пишем в файл server/logs
        try { File.AppendAllText(Path.Combine(_logs, $"{tabKey}.log"), line + Environment.NewLine); } catch { }
    }

    private void SetStatus(string text)
    {
        TxtStatus.Text = text;
        TxtStatus.Foreground = text.Contains("ошибка", StringComparison.OrdinalIgnoreCase)
            ? ErrorBrush : InfoBrush;
    }

    private void RefreshStats()
    {
        var online = _db.OnlinePlayers();
        TxtOnline.Text = online < 0 ? "—" : online.ToString();
        TxtAccounts.Text = Math.Max(0, _db.TotalAccounts()).ToString();
        TxtChars.Text = Math.Max(0, _db.TotalCharacters()).ToString();
        var build = _db.RealmGameBuild();
        TxtBuild.Text = build == 0 ? "26124" : build.ToString();
        TxtBuild.Foreground = build == 26124 || build == 0 ? OkBrush : ErrorBrush;
    }

    private void BtnStart_Click(object sender, RoutedEventArgs e)
    {
        _world.AutoRestart = ChkAutoRestart.IsChecked == true;
        SetStatus("Запускаю стек LEGIONFORGE: MySQL -> BNetServer -> WorldServer ...");
        _mysql.Start();
        Task.Delay(4000).ContinueWith(_ => Dispatcher.BeginInvoke(() =>
        {
            _bnet.Start();
            Task.Delay(2000).ContinueWith(_2 => Dispatcher.BeginInvoke(() =>
            {
                _world.Start();
                SetStatus("Сервер запущен. Логи поступают во вкладки.");
            }));
        }));
    }

    private void BtnStop_Click(object sender, RoutedEventArgs e)
    {
        SetStatus("Останавливаю сервер ...");
        _world.Stop();
        _bnet.Stop();
        _mysql.Stop();
    }

    private void BtnReboot_Click(object sender, RoutedEventArgs e) => _world.SoftRestart(60);

    private void BtnSave_Click(object sender, RoutedEventArgs e)
    {
        _world.SendCommand("saveall");
        _db.FlushDatabases();
        SetStatus("Все игроки сохранены, таблицы БД сброшены на диск.");
    }

    private void BtnBackup_Click(object sender, RoutedEventArgs e)
    {
        SetStatus("Делаю бэкап баз данных ...");
        var dump = Path.Combine(_root, "tools", "mysql", "bin", "mysqldump.exe");
        var target = Path.Combine(_root, "server", "backups");
        Task.Run(() => _db.Backup(dump, target)).ContinueWith(t => Dispatcher.BeginInvoke(() =>
            SetStatus(t.IsCompletedSuccessfully ? $"Бэкап готов: {t.Result}" : "Бэкап не удался.")));
    }

    private void BtnPanel_Click(object sender, RoutedEventArgs e)
    {
        try { Process.Start(new ProcessStartInfo("http://127.0.0.1:3000") { UseShellExecute = true }); }
        catch (Exception ex) { SetStatus("Не удалось открыть панель: " + ex.Message); }
    }

    private void BtnSendCommand_Click(object sender, RoutedEventArgs e) => SendCommand();

    private void TxtCommand_KeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter) SendCommand();
    }

    private void SendCommand()
    {
        var cmd = TxtCommand.Text.Trim();
        if (cmd.Length == 0) return;
        AppendLine("WorldServer", "GM", "> " + cmd);
        _world.SendCommand(cmd);
        TxtCommand.Clear();
    }

    private void Window_Closing(object? sender, System.ComponentModel.CancelEventArgs e)
    {
        _statsTimer.Stop();
        _world.Stop(); _bnet.Stop(); _mysql.Stop();
        _world.Dispose(); _bnet.Dispose(); _mysql.Dispose();
    }
}
