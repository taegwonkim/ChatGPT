using System.Net;

namespace MeasurementMonitor;

public class WifiPanel : GroupBox
{
    private readonly TextBox ssid = new();
    private readonly TextBox password = new() { UseSystemPasswordChar = true };
    private readonly TextBox serverIp = new() { Text = "192.168.0.100" };
    private readonly NumericUpDown serverPort = new() { Minimum = 1, Maximum = 65535, Value = 5000 };
    private readonly CheckBox dhcp = new() { Text = "DHCP On", Checked = true, AutoSize = true };
    private readonly TextBox localIp = new() { Text = "192.168.0.50" };
    private readonly TextBox gateway = new() { Text = "192.168.0.1" };
    private readonly TextBox netmask = new() { Text = "255.255.255.0" };
    internal event EventHandler? ReadRequested;
    internal event EventHandler<WifiSettings>? WriteRequested;

    public WifiPanel()
    {
        Text = "Wi-Fi 설정";
        Dock = DockStyle.Fill;
        Padding = new Padding(10);
        var grid = Ui.Grid();
        Ui.AddRow(grid, "AP SSID", ssid, "Password", password);
        Ui.AddRow(grid, "Server IP", serverIp, "Server Port", serverPort);
        Ui.AddRow(grid, "DHCP", dhcp, "Local IP", localIp);
        Ui.AddRow(grid, "Gateway", gateway, "Net Mask", netmask);
        var buttons = Ui.Buttons(("Read", () => ReadRequested?.Invoke(this, EventArgs.Empty)),
            ("Write", Write));
        grid.Controls.Add(buttons, 0, 4); grid.SetColumnSpan(buttons, 4);
        Controls.Add(grid);
        dhcp.CheckedChanged += (_, _) => UpdateDhcpFields();
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
