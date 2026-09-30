using MySql.Data.MySqlClient;

namespace LegionForge.Manager.Services;

/// <summary>
/// LEGIONFORGE :: быстрый доступ к auth/characters/world (MySQL 8.0)
/// для кнопок «Сохранить всех игроков и БД», «Бэкап БД» и статистики онлайна.
/// </summary>
public sealed class GameDatabase
{
    private readonly string _connectionString;

    public GameDatabase(string host, int port, string user, string password)
    {
        _connectionString = $"Server={host};Port={port};Uid={user};Pwd={password};SslMode=None;AllowUserVariables=True;ConnectionTimeout=5;";
    }

    private MySqlConnection Open(string database)
    {
        var cs = new MySqlConnectionStringBuilder(_connectionString) { Database = database }.ConnectionString;
        var conn = new MySqlConnection(cs);
        conn.Open();
        return conn;
    }

    public int OnlinePlayers()
    {
        try
        {
            using var conn = Open("legionforge_auth");
            using var cmd = new MySqlCommand("SELECT COUNT(*) FROM account WHERE online = 1;", conn);
            return Convert.ToInt32(cmd.ExecuteScalar() ?? 0);
        }
        catch { return -1; }
    }

    public int TotalAccounts()
    {
        try
        {
            using var conn = Open("legionforge_auth");
            using var cmd = new MySqlCommand("SELECT COUNT(*) FROM account;", conn);
            return Convert.ToInt32(cmd.ExecuteScalar() ?? 0);
        }
        catch { return -1; }
    }

    public int TotalCharacters()
    {
        try
        {
            using var conn = Open("legionforge_characters");
            using var cmd = new MySqlCommand("SELECT COUNT(*) FROM characters;", conn);
            return Convert.ToInt32(cmd.ExecuteScalar() ?? 0);
        }
        catch { return -1; }
    }

    public int RealmGameBuild()
    {
        try
        {
            using var conn = Open("legionforge_auth");
            using var cmd = new MySqlCommand("SELECT gamebuild FROM realmlist WHERE id = 1;", conn);
            var v = cmd.ExecuteScalar();
            return v is null or DBNull ? 0 : Convert.ToInt32(v);
        }
        catch { return 0; }
    }

    /// <summary>Принудительное сохранение всех игроков и БД (через worldserver-консоль).</summary>
    public void FlushDatabases()
    {
        foreach (var db in new[] { "legionforge_auth", "legionforge_characters", "legionforge_world" })
        {
            try
            {
                using var conn = Open(db);
                using var cmd = new MySqlCommand("FLUSH TABLES;", conn);
                cmd.ExecuteNonQuery();
            }
            catch { /* база может быть недоступна - не критично */ }
        }
    }

    /// <summary>Бэкап всех баз в server/backups/ через mysqldump.</summary>
    public string Backup(string mysqldumpPath, string targetDirectory)
    {
        Directory.CreateDirectory(targetDirectory);
        var stamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
        var file = Path.Combine(targetDirectory, $"legionforge_backup_{stamp}.sql");
        var psi = new System.Diagnostics.ProcessStartInfo
        {
            FileName = mysqldumpPath,
            Arguments = "--host=127.0.0.1 --port=3306 --user=root --password=legionforge " +
                        "--databases legionforge_auth legionforge_characters legionforge_world " +
                        $"--result-file=\"{file}\" --single-transaction --routines --triggers",
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardError = true
        };
        using var p = System.Diagnostics.Process.Start(psi);
        p?.WaitForExit(300000);
        return file;
    }
}
