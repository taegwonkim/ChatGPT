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

    internal static bool TryParseWifiSettings(string frame, out WifiSettings? value)
    {
        value = null;
        string[] fields = frame.Split(',');
        if (fields.Length != 9 || (fields[0] != "WIFI_W_ALL" && fields[0] != "WIFI_R_ALL") ||
            !int.TryParse(fields[4], NumberStyles.None, CultureInfo.InvariantCulture, out int port) ||
            port is < 1 or > 65535 || (fields[5] != "0" && fields[5] != "1")) return false;
        value = new(fields[1], fields[2], fields[3], port, fields[5] == "1", fields[6], fields[7], fields[8]);
        return true;
    }

    internal static bool TryParseMeasurementSettings(string frame, out MeasurementSettings? value)
    {
        value = null;
        string[] fields = frame.Split(',');
        if (fields.Length != 5 || (fields[0] != "MEAS_W_ALL" && fields[0] != "MEAS_R_ALL") ||
            !decimal.TryParse(fields[1], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal reference) ||
            !decimal.TryParse(fields[2], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal offset) ||
            !decimal.TryParse(fields[3], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal resistance) ||
            !decimal.TryParse(fields[4], NumberStyles.Number, CultureInfo.InvariantCulture, out decimal interval)) return false;
        value = new(reference, offset, resistance, interval);
        return true;
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

internal sealed class ProtocolFramer
{
    private readonly List<byte> payload = [];
    private bool receiving;
    private bool sawCr;

    internal IEnumerable<string> Push(ReadOnlySpan<byte> bytes)
    {
        var frames = new List<string>();
        foreach (byte value in bytes)
        {
            if (value == DeviceProtocol.Stx)
            {
                payload.Clear();
                receiving = true;
                sawCr = false;
                continue;
            }
            if (!receiving)
                continue;
            if (sawCr)
            {
                if (value == (byte)'\n')
                    frames.Add(Encoding.ASCII.GetString(payload.ToArray()));
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
