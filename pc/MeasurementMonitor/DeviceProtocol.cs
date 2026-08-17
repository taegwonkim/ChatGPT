using System.Globalization;
using System.Text;

namespace MeasurementMonitor;

internal static class DeviceProtocol
{
    internal const byte Stx = 0x02;

    internal static byte[] WifiRead() => Frame("WIFI_R_ALL");

    internal static byte[] WifiWrite(WifiSettings value) => Frame(string.Join(',',
        "WIFI_W_ALL", Escape(value.Ssid), Escape(value.Password),
        Escape(value.ServerIp), value.ServerPort.ToString(CultureInfo.InvariantCulture),
        value.Dhcp ? "1" : "0", Escape(value.LocalIp), Escape(value.Gateway),
        Escape(value.Netmask)));

    internal static byte[] MeasurementRead() => Frame("MEAS_R_ALL");

    internal static byte[] MeasurementWrite(MeasurementSettings value) => Frame(string.Join(',',
        "MEAS_W_ALL",
        value.ReferenceMv.ToString(CultureInfo.InvariantCulture),
        value.OffsetMv.ToString(CultureInfo.InvariantCulture),
        value.ResistanceMilliOhm.ToString(CultureInfo.InvariantCulture),
        value.IntervalSeconds.ToString(CultureInfo.InvariantCulture)));

    internal static byte[] Frame(string payload) =>
        [Stx, .. Encoding.ASCII.GetBytes(payload), (byte)'\r', (byte)'\n'];

    internal static bool TryParseWifiSettings(string frame, out WifiSettings? value,
        bool allowPayloadOnly = false)
    {
        value = null;
        if (!TryGetPayload(frame, "WIFI_R_ALL", "WIFI_W_ALL", allowPayloadOnly,
                8, out string[] fields) ||
            !int.TryParse(fields[3], NumberStyles.None, CultureInfo.InvariantCulture, out int port) ||
            port is < 1 or > 65535 || !TryParseBoolean(fields[4], out bool dhcp)) return false;
        value = new(fields[0], fields[1], fields[2], port, dhcp, fields[5], fields[6], fields[7]);
        return true;
    }

    internal static bool TryParseMeasurementSettings(string frame, out MeasurementSettings? value,
        bool allowPayloadOnly = false)
    {
        value = null;
        if (!TryGetPayload(frame, "MEAS_R_ALL", "MEAS_W_ALL", allowPayloadOnly,
                4, out string[] fields) ||
            !decimal.TryParse(fields[0], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal reference) ||
            !decimal.TryParse(fields[1], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal offset) ||
            !decimal.TryParse(fields[2], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal resistance) ||
            !decimal.TryParse(fields[3], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal interval)) return false;
        value = new(reference, offset, resistance, interval);
        return true;
    }

    internal static bool IsMeasurementData(string frame)
    {
        string[] fields = frame.Split(',', StringSplitOptions.TrimEntries);
        if (fields.Length > 1 && (fields[0].Equals("DATA", StringComparison.OrdinalIgnoreCase) ||
            fields[0].Equals("MEAS_DATA", StringComparison.OrdinalIgnoreCase))) fields = fields[1..];
        return fields.Length > 0 && fields.All(field =>
            decimal.TryParse(field, NumberStyles.Float, CultureInfo.InvariantCulture, out _));
    }

    private static bool TryGetPayload(string frame, string readCommand, string writeCommand,
        bool allowPayloadOnly, int expectedFields, out string[] fields)
    {
        string text = frame.Trim();
        bool tagged = false;
        foreach (string command in new[] { readCommand, writeCommand })
        {
            if (!text.StartsWith(command, StringComparison.OrdinalIgnoreCase)) continue;
            text = text[command.Length..].TrimStart(' ', ',', ':', '=');
            tagged = true;
            break;
        }
        fields = text.Split(',', StringSplitOptions.TrimEntries);
        return (tagged || allowPayloadOnly) && fields.Length == expectedFields;
    }

    private static bool TryParseBoolean(string text, out bool value)
    {
        if (text.Equals("1") || text.Equals("ON", StringComparison.OrdinalIgnoreCase) ||
            text.Equals("TRUE", StringComparison.OrdinalIgnoreCase)) { value = true; return true; }
        if (text.Equals("0") || text.Equals("OFF", StringComparison.OrdinalIgnoreCase) ||
            text.Equals("FALSE", StringComparison.OrdinalIgnoreCase)) { value = false; return true; }
        value = false;
        return false;
    }

    // CSV 필드에 쉼표가 들어오면 MCU parser가 모호해지므로 설정 입력에서 차단합니다.
    private static string Escape(string value)
    {
        if (value.IndexOfAny([',', '\r', '\n', (char)Stx]) >= 0)
            throw new ArgumentException("설정 문자열에는 쉼표, 줄바꿈, STX를 사용할 수 없습니다.");
        return value;
    }
}

internal sealed record WifiSettings(string Ssid, string Password, string ServerIp,
    int ServerPort, bool Dhcp, string LocalIp, string Gateway, string Netmask);

internal sealed record MeasurementSettings(decimal ReferenceMv, decimal OffsetMv,
    decimal ResistanceMilliOhm, decimal IntervalSeconds);

internal readonly record struct ReceivedFrame(string Payload, bool HasStx);

internal sealed class ProtocolFramer
{
    private readonly List<byte> payload = [];
    private readonly List<byte> unframed = [];
    private bool receiving;
    private bool sawCr;
    private bool sawUnframedCr;

    internal IEnumerable<ReceivedFrame> Push(ReadOnlySpan<byte> bytes)
    {
        var frames = new List<ReceivedFrame>();
        foreach (byte value in bytes)
        {
            if (value == DeviceProtocol.Stx)
            {
                payload.Clear();
                unframed.Clear();
                receiving = true;
                sawCr = false;
                sawUnframedCr = false;
                continue;
            }
            if (!receiving)
            {
                if (sawUnframedCr)
                {
                    if (value == (byte)'\n' && unframed.Count != 0)
                        frames.Add(new(Encoding.ASCII.GetString(unframed.ToArray()), false));
                    unframed.Clear();
                    sawUnframedCr = false;
                    if (value == (byte)'\n') continue;
                }
                if (value == (byte)'\r')
                {
                    sawUnframedCr = true;
                    continue;
                }
                if (unframed.Count >= 4096) unframed.Clear();
                unframed.Add(value);
                continue;
            }
            if (sawCr)
            {
                if (value == (byte)'\n')
                    frames.Add(new(Encoding.ASCII.GetString(payload.ToArray()), true));
                receiving = false;
                sawCr = false;
                continue;
            }
            if (value == (byte)'\r')
            {
                sawCr = true;
                continue;
            }
            if (payload.Count >= 4096)
            {
                payload.Clear();
                receiving = false;
                continue;
            }
            payload.Add(value);
        }
        return frames;
    }
}
