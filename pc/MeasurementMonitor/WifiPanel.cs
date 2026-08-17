using System.Net;

namespace MeasurementMonitor;

public partial class WifiPanel : UserControl
{
    internal event EventHandler? ReadRequested;
    internal event EventHandler<WifiSettings>? WriteRequested;

    public WifiPanel()
    {
        InitializeComponent();
        dhcp.CheckedChanged += (_, _) => UpdateDhcpFields();
        readButton.Click += (_, _) => ReadRequested?.Invoke(this, EventArgs.Empty);
        writeButton.Click += (_, _) => Write();
        UpdateDhcpFields();
    }

    private void UpdateDhcpFields() => localIp.Enabled = gateway.Enabled = netmask.Enabled = !dhcp.Checked;
    internal void Apply(WifiSettings value)
    {
        ssid.Text = value.Ssid; password.Text = value.Password; serverIp.Text = value.ServerIp;
        serverPort.Value = Math.Clamp(value.ServerPort, 1, 65535); dhcp.Checked = value.Dhcp;
        localIp.Text = value.LocalIp; gateway.Text = value.Gateway; netmask.Text = value.Netmask;
    }
    internal WifiSettings CurrentSettings => new(ssid.Text, password.Text, serverIp.Text,
        decimal.ToInt32(serverPort.Value), dhcp.Checked, localIp.Text, gateway.Text, netmask.Text);
    private void Write()
    {
        if (!IPAddress.TryParse(serverIp.Text.Trim(), out _) ||
            (!dhcp.Checked && (!IPAddress.TryParse(localIp.Text.Trim(), out _) ||
             !IPAddress.TryParse(gateway.Text.Trim(), out _) || !IPAddress.TryParse(netmask.Text.Trim(), out _))))
        { MessageBox.Show("IP 주소 형식을 확인하십시오.", "입력 오류"); return; }
        WriteRequested?.Invoke(this, new(ssid.Text.Trim(), password.Text, serverIp.Text.Trim(),
            decimal.ToInt32(serverPort.Value), dhcp.Checked, localIp.Text.Trim(), gateway.Text.Trim(), netmask.Text.Trim()));
    }
}
