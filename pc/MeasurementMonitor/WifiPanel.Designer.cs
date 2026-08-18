namespace MeasurementMonitor;

partial class WifiPanel
{
    private System.ComponentModel.IContainer? components;
    private GroupBox wifiGroup = null!;
    private TableLayoutPanel grid = null!;
    private TextBox ssid = null!;
    private TextBox password = null!;
    private TextBox serverIp = null!;
    private NumericUpDown serverPort = null!;
    private CheckBox dhcp = null!;
    private TextBox localIp = null!;
    private TextBox gateway = null!;
    private TextBox netmask = null!;
    private Button readButton = null!;
    private Button writeButton = null!;
    private Label ssidLabel = null!;
    private Label passwordLabel = null!;
    private Label serverIpLabel = null!;
    private Label serverPortLabel = null!;
    private Label dhcpLabel = null!;
    private Label localIpLabel = null!;
    private Label gatewayLabel = null!;
    private Label netmaskLabel = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container();
        wifiGroup = new GroupBox();
        grid = new TableLayoutPanel();
        ssid = new TextBox();
        password = new TextBox();
        serverIp = new TextBox();
        serverPort = new NumericUpDown();
        dhcp = new CheckBox();
        localIp = new TextBox();
        gateway = new TextBox();
        netmask = new TextBox();
        readButton = new Button();
        writeButton = new Button();
        ssidLabel = MakeLabel("AP SSID");
        passwordLabel = MakeLabel("Password");
        serverIpLabel = MakeLabel("Server IP");
        serverPortLabel = MakeLabel("Server Port");
        dhcpLabel = MakeLabel("DHCP");
        localIpLabel = MakeLabel("Local IP");
        gatewayLabel = MakeLabel("Gateway");
        netmaskLabel = MakeLabel("Net Mask");
        ((System.ComponentModel.ISupportInitialize)serverPort).BeginInit();
        wifiGroup.SuspendLayout();
        grid.SuspendLayout();
        SuspendLayout();

        wifiGroup.Controls.Add(grid);
        wifiGroup.Dock = DockStyle.Fill;
        wifiGroup.Location = new Point(0, 0);
        wifiGroup.Name = "wifiGroup";
        wifiGroup.Padding = new Padding(10);
        wifiGroup.Size = new Size(520, 230);
        wifiGroup.TabIndex = 0;
        wifiGroup.TabStop = false;
        wifiGroup.Text = "Wi-Fi 설정";

        grid.ColumnCount = 4;
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90F));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90F));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
        grid.Controls.Add(ssidLabel, 0, 0);
        grid.Controls.Add(ssid, 1, 0);
        grid.Controls.Add(passwordLabel, 2, 0);
        grid.Controls.Add(password, 3, 0);
        grid.Controls.Add(serverIpLabel, 0, 1);
        grid.Controls.Add(serverIp, 1, 1);
        grid.Controls.Add(serverPortLabel, 2, 1);
        grid.Controls.Add(serverPort, 3, 1);
        grid.Controls.Add(dhcpLabel, 0, 2);
        grid.Controls.Add(dhcp, 1, 2);
        grid.Controls.Add(localIpLabel, 2, 2);
        grid.Controls.Add(localIp, 3, 2);
        grid.Controls.Add(gatewayLabel, 0, 3);
        grid.Controls.Add(gateway, 1, 3);
        grid.Controls.Add(netmaskLabel, 2, 3);
        grid.Controls.Add(netmask, 3, 3);
        grid.Controls.Add(readButton, 1, 4);
        grid.Controls.Add(writeButton, 2, 4);
        grid.Dock = DockStyle.Fill;
        grid.Location = new Point(10, 26);
        grid.Name = "grid";
        grid.RowCount = 5;
        grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
        grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
        grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
        grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
        grid.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
        grid.Size = new Size(500, 194);
        grid.TabIndex = 0;

        ssid.Anchor = AnchorStyles.Left | AnchorStyles.Right;
        ssid.Name = "ssid";
        password.Anchor = AnchorStyles.Left | AnchorStyles.Right;
        password.Name = "password";
        password.UseSystemPasswordChar = true;
        serverIp.Anchor = AnchorStyles.Left | AnchorStyles.Right;
        serverIp.Name = "serverIp";
        serverIp.Text = "192.168.0.100";
        serverPort.Anchor = AnchorStyles.Left | AnchorStyles.Right;
        serverPort.Maximum = 65535;
        serverPort.Minimum = 1;
        serverPort.Name = "serverPort";
        serverPort.Value = 5000;
        dhcp.AutoSize = true;
        dhcp.Checked = true;
        dhcp.CheckState = CheckState.Checked;
        dhcp.Name = "dhcp";
        dhcp.Text = "DHCP On";
        localIp.Anchor = AnchorStyles.Left | AnchorStyles.Right;
        localIp.BorderStyle = BorderStyle.FixedSingle;
        localIp.Name = "localIp";
        localIp.Text = "192.168.0.50";
        gateway.Anchor = AnchorStyles.Left | AnchorStyles.Right;
        gateway.BorderStyle = BorderStyle.FixedSingle;
        gateway.Name = "gateway";
        gateway.Text = "192.168.0.1";
        netmask.Anchor = AnchorStyles.Left | AnchorStyles.Right;
        netmask.BorderStyle = BorderStyle.FixedSingle;
        netmask.Name = "netmask";
        netmask.Text = "255.255.255.0";
        readButton.Anchor = AnchorStyles.None;
        readButton.AutoSize = true;
        readButton.Name = "readButton";
        readButton.Text = "Read";
        writeButton.Anchor = AnchorStyles.None;
        writeButton.AutoSize = true;
        writeButton.Name = "writeButton";
        writeButton.Text = "Write";

        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode = AutoScaleMode.Font;
        Controls.Add(wifiGroup);
        MinimumSize = new Size(440, 210);
        Name = "WifiPanel";
        Size = new Size(520, 230);
        ((System.ComponentModel.ISupportInitialize)serverPort).EndInit();
        grid.ResumeLayout(false);
        grid.PerformLayout();
        wifiGroup.ResumeLayout(false);
        ResumeLayout(false);
    }

    private static Label MakeLabel(string text) => new Label
    {
        Anchor = AnchorStyles.Left | AnchorStyles.Right,
        AutoSize = false,
        Height = 23,
        Margin = new Padding(3),
        TextAlign = ContentAlignment.MiddleLeft,
        Text = text
    };
}
