using System.Windows;

namespace LegionForge.Manager;

/// <summary>
/// LEGIONFORGE Manager :: точка входа WPF-приложения.
/// Глобальная обработка исключений, чтобы падение одного потока
/// не убивало панель управления сервером.
/// </summary>
public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);
        DispatcherUnhandledException += (_, args) =>
        {
            MessageBox.Show("LEGIONFORGE Manager: " + args.Exception.Message,
                "Ошибка интерфейса", MessageBoxButton.OK, MessageBoxImage.Warning);
            args.Handled = true;
        };
    }
}
