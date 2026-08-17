using System.Text.Json;

namespace MeasurementMonitor;

internal sealed record SerialSettings(string PortName, int BaudRate, int Timeout);
internal sealed record LayoutSettings(int SettingsHeight, int WifiWidth);
internal sealed record SavedAppSettings(WifiSettings? Wifi, MeasurementSettings? Measurement,
    SerialSettings? Serial, LayoutSettings? Layout);

internal static class AppSettingsStore
{
    private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };
    internal static string FilePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "STM32MeasurementMonitor", "settings.json");

    internal static SavedAppSettings Load()
    {
        try
        {
            return File.Exists(FilePath)
                ? JsonSerializer.Deserialize<SavedAppSettings>(File.ReadAllText(FilePath), JsonOptions)
                    ?? new(null, null, null, null)
                : new(null, null, null, null);
        }
        catch (JsonException) { return new(null, null, null, null); }
        catch (IOException) { return new(null, null, null, null); }
        catch (UnauthorizedAccessException) { return new(null, null, null, null); }
    }

    internal static bool Save(SavedAppSettings settings)
    {
        try
        {
            string? directory = Path.GetDirectoryName(FilePath);
            if (directory is null) return false;
            Directory.CreateDirectory(directory);
            string temporary = FilePath + ".tmp";
            File.WriteAllText(temporary, JsonSerializer.Serialize(settings, JsonOptions));
            File.Move(temporary, FilePath, true);
            return true;
        }
        catch (IOException) { return false; }
        catch (UnauthorizedAccessException) { return false; }
    }
}
