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

    internal static byte[] ResetRead(ResetTimeUnit unit) =>
        Frame(unit == ResetTimeUnit.Hours ? "RTC_R_H" : "RTC_R_M");

    internal static byte[] ResetWrite(ResetPeriodSetting value) => Frame(string.Join(',',
        value.Unit == ResetTimeUnit.Hours ? "RTC_W_H" : "RTC_W_M",
        value.Value.ToString(CultureInfo.InvariantCulture)));

    internal static bool TryParseResetSettings(string frame, out ResetPeriodSetting value)
    {
        value = default;
        string text = frame.Trim();
        foreach ((string command, ResetTimeUnit unit, uint maximum) in new[]
        {
            ("RTC_R_M", ResetTimeUnit.Minutes, 525600U),
            ("RTC_W_M", ResetTimeUnit.Minutes, 525600U),
            ("RTC_R_H", ResetTimeUnit.Hours, 8760U),
            ("RTC_W_H", ResetTimeUnit.Hours, 8760U)
        })
        {
            if (!text.StartsWith(command, StringComparison.OrdinalIgnoreCase)) continue;
            text = text[command.Length..].TrimStart(' ', ',', ':', '=');
            if (!uint.TryParse(text.TrimEnd(','), NumberStyles.None,
                CultureInfo.InvariantCulture, out uint period) || period > maximum) return false;
            value = new(period, unit);
            return true;
        }
        return false;
    }

    internal static bool TryParseMacAddress(string frame, out string macAddress)
    {
        macAddress = string.Empty;
        string text = frame.Trim('\0', ' ', '\t', '\r', '\n', (char)Stx);
        foreach (string prefix in new[] { "MAC_ADDRESS", "MAC ADDRESS", "MAC" })
        {
            if (!text.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) continue;
            if (text.Length == prefix.Length) return false;
            char separator = text[prefix.Length];
            if (separator is not ('_' or ',' or ':' or '=' or ' ')) return false;
            macAddress = text[(prefix.Length + 1)..].TrimStart('_', ',', ':', '=', ' ').Trim();
            return macAddress.Length != 0;
        }
        return false;
    }

    internal static byte[] MeasurementWrite(MeasurementSettings value) => Frame(string.Join(',',
        "MEAS_W_ALL",
        value.ReferenceMv.ToString(CultureInfo.InvariantCulture),
        value.OffsetMv.ToString(CultureInfo.InvariantCulture),
        value.ResistanceMilliOhm.ToString(CultureInfo.InvariantCulture),
        value.IntervalSeconds.ToString(CultureInfo.InvariantCulture),
        value.Rs485Only ? "1" : "0"));

    internal static byte[] Frame(string payload) =>
        [Stx, .. Encoding.ASCII.GetBytes(payload), (byte)'\r', (byte)'\n'];

    internal static bool TryParseWifiSettings(string frame, out WifiSettings? value,
        bool allowPayloadOnly = false)
    {
        value = null;
        if (!TryGetPayload(frame, "WIFI_R_ALL", "WIFI_W_ALL", allowPayloadOnly,
                8, out string[] fields) ||
            !int.TryParse(CleanValue(fields[3]), NumberStyles.None, CultureInfo.InvariantCulture, out int port) ||
            port is < 1 or > 65535 || !TryParseBoolean(CleanValue(fields[4]), out bool dhcp)) return false;
        for (int index = 0; index < fields.Length; index++) fields[index] = CleanValue(fields[index]);
        value = new(fields[0], fields[1], fields[2], port, dhcp, fields[5], fields[6], fields[7]);
        return true;
    }

    internal static bool TryParseMeasurementSettings(string frame, out MeasurementSettings? value,
        bool allowPayloadOnly = false)
    {
        value = null;
        bool validFields = TryGetPayload(frame, "MEAS_R_ALL", "MEAS_W_ALL",
            allowPayloadOnly, 5, out string[] fields);
        if (!validFields)
            validFields = TryGetPayload(frame, "MEAS_R_ALL", "MEAS_W_ALL",
                allowPayloadOnly, 4, out fields); // 이전 저장 장치 응답 호환
        if (!validFields ||
            !decimal.TryParse(CleanValue(fields[0]), NumberStyles.Number, CultureInfo.InvariantCulture, out decimal reference) ||
            !decimal.TryParse(CleanValue(fields[1]), NumberStyles.Number, CultureInfo.InvariantCulture, out decimal offset) ||
            !decimal.TryParse(CleanValue(fields[2]), NumberStyles.Number, CultureInfo.InvariantCulture, out decimal resistance) ||
            !decimal.TryParse(CleanValue(fields[3]), NumberStyles.Number, CultureInfo.InvariantCulture, out decimal interval)) return false;
        bool rs485Only = false;
        if (fields.Length == 5 && !TryParseBoolean(CleanValue(fields[4]), out rs485Only))
            return false;
        value = new(reference, offset, resistance, interval, rs485Only);
        return true;
    }

    internal static bool IsMeasurementData(string frame)
    {
        // MCU 측정 frame의 정식 식별자: <STX>DC_...<CR><LF>
        if (frame.StartsWith("DC_", StringComparison.OrdinalIgnoreCase)) return true;

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
            text = text[command.Length..].TrimStart(' ', ',', ';', '|', ':', '=');
            tagged = true;
            break;
        }
        fields = text.Split([',', ';', '|'], StringSplitOptions.TrimEntries);
        // MCU가 마지막 항목 뒤에도 ','를 붙이는 응답 형식을 허용합니다.
        if (fields.Length == expectedFields + 1 && fields[^1].Length == 0)
            fields = fields[..^1];
        return (tagged || allowPayloadOnly) && fields.Length == expectedFields;
    }

    private static string CleanValue(string field)
    {
        string value = field.Trim();
        int separator = value.IndexOfAny(['=', ':']);
        if (separator > 0 && IsSettingKey(value[..separator]))
            value = value[(separator + 1)..].Trim();
        return value.Trim('"');
    }

    private static bool IsSettingKey(string text) => text.Trim().ToUpperInvariant() is
        "SSID" or "PASSWORD" or "PASS" or "SERVER_IP" or "SERVERIP" or
        "SERVER_PORT" or "SERVERPORT" or "PORT" or "DHCP" or "IP" or
        "LOCAL_IP" or "LOCALIP" or "GATEWAY" or "GW" or "NETMASK" or "NET_MASK" or
        "REFERENCE" or "REFERENCE_MV" or "OFFSET" or "OFFSET_MV" or
        "RESISTANCE" or "RESISTANCE_MOHM" or "INTERVAL" or "INTERVAL_TIME";

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
    decimal ResistanceMilliOhm, decimal IntervalSeconds, bool Rs485Only = false);

internal enum ResetTimeUnit { Minutes, Hours }
internal readonly record struct ResetPeriodSetting(uint Value, ResetTimeUnit Unit)
{
    internal uint TotalSeconds => checked(Value * (Unit == ResetTimeUnit.Hours ? 3600U : 60U));
}

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
