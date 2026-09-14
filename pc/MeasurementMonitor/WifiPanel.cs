using System.Net;

namespace MeasurementMonitor;

public partial class WifiPanel : UserControl
{
    internal event EventHandler? ReadRequested;
    internal event EventHandler<WifiSettings>? WriteRequested;

    public WifiPanel()
    {
        InitializeComponent();
        EnsureReadButtonEnabled();
        dhcp.CheckedChanged += (_, _) => UpdateDhcpFields();
        readButton.Click += (_, _) => ReadRequested?.Invoke(this, EventArgs.Empty);
        writeButton.Click += (_, _) => Write();
        UpdateDhcpFields();
    }

    /* 사용자 layout 복원이나 부모 상태 변경 뒤에도 Read 명령은 항상 사용할 수 있습니다. */
    internal void EnsureReadButtonEnabled()
    {
        wifiGroup.Enabled = true;
        grid.Enabled = true;
        readButton.Enabled = true;
        readButton.TabStop = true;
        readButton.UseVisualStyleBackColor = true;
    }

    private void UpdateDhcpFields()
    {
        bool readOnly = dhcp.Checked;
        SetStaticIpFieldState(localIp, readOnly);
        SetStaticIpFieldState(gateway, readOnly);
        SetStaticIpFieldState(netmask, readOnly);
    }

    private static void SetStaticIpFieldState(TextBox field, bool readOnly)
    {
        // Enabled=false는 Windows theme에서 테두리까지 흐리게 그리므로 사용하지 않습니다.
        field.Enabled = true;
        field.ReadOnly = readOnly;
        field.TabStop = !readOnly;
        field.BorderStyle = BorderStyle.FixedSingle;
        field.BackColor = SystemColors.Window;
        field.ForeColor = readOnly ? SystemColors.GrayText : SystemColors.WindowText;
    }
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
