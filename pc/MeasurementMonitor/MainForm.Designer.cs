namespace MeasurementMonitor;

partial class MainForm
{
    private System.ComponentModel.IContainer? components;
    private SerialPanel serialPanel = null!;
    private WifiPanel wifiPanel = null!;
    private MeasurementPanel measurementPanel = null!;
    private MonitorPanel monitorPanel = null!;
    private TableLayoutPanel settingsLayout = null!;
    private ToolTip toolTip = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container();
        serialPanel = new SerialPanel();
        wifiPanel = new WifiPanel();
        measurementPanel = new MeasurementPanel();
        monitorPanel = new MonitorPanel();
        settingsLayout = new TableLayoutPanel();
        toolTip = new ToolTip(components);
        settingsLayout.SuspendLayout();
        SuspendLayout();

        serialPanel.Dock = DockStyle.Top;
        serialPanel.Location = new Point(0, 0);
        serialPanel.Name = "serialPanel";
        serialPanel.Size = new Size(984, 75);
        serialPanel.TabIndex = 0;
        toolTip.SetToolTip(serialPanel, "COM port 연결과 수신 화면 지우기");

        settingsLayout.ColumnCount = 2;
        settingsLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 55F));
        settingsLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 45F));
        settingsLayout.Controls.Add(wifiPanel, 0, 0);
        settingsLayout.Controls.Add(measurementPanel, 1, 0);
        settingsLayout.Dock = DockStyle.Top;
        settingsLayout.Location = new Point(0, 75);
        settingsLayout.Name = "settingsLayout";
        settingsLayout.RowCount = 1;
        settingsLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
        settingsLayout.Size = new Size(984, 245);
        settingsLayout.TabIndex = 1;

        wifiPanel.Dock = DockStyle.Fill;
        wifiPanel.Name = "wifiPanel";
        wifiPanel.TabIndex = 0;
        toolTip.SetToolTip(wifiPanel, "AP/Server/DHCP 설정 Read 및 Write");
        measurementPanel.Dock = DockStyle.Fill;
        measurementPanel.Name = "measurementPanel";
        measurementPanel.TabIndex = 1;
        toolTip.SetToolTip(measurementPanel, "측정 조건 설정 Read 및 Write");

        monitorPanel.Dock = DockStyle.Fill;
        monitorPanel.Location = new Point(0, 320);
        monitorPanel.Name = "monitorPanel";
        monitorPanel.Size = new Size(984, 341);
        monitorPanel.TabIndex = 2;
        toolTip.SetToolTip(monitorPanel, "MCU 측정값 및 STATUS frame 표시");

        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode = AutoScaleMode.Font;
        ClientSize = new Size(984, 661);
        Controls.Add(monitorPanel);
        Controls.Add(settingsLayout);
        Controls.Add(serialPanel);
        MinimumSize = new Size(900, 650);
        Name = "MainForm";
        StartPosition = FormStartPosition.CenterScreen;
        Text = "STM32 Measurement Monitor - Visual Studio 2022";
        settingsLayout.ResumeLayout(false);
        ResumeLayout(false);
    }
}
