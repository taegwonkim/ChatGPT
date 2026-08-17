namespace MeasurementMonitor;

partial class MainForm
{
    private System.ComponentModel.IContainer? components;
    private SerialPanel serialPanel = null!;
    private WifiPanel wifiPanel = null!;
    private MeasurementPanel measurementPanel = null!;
    private MonitorPanel monitorPanel = null!;
    private SplitContainer contentSplit = null!;
    private SplitContainer settingsSplit = null!;
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
        contentSplit = new SplitContainer();
        settingsSplit = new SplitContainer();
        toolTip = new ToolTip(components);
        ((System.ComponentModel.ISupportInitialize)contentSplit).BeginInit();
        contentSplit.Panel1.SuspendLayout();
        contentSplit.Panel2.SuspendLayout();
        contentSplit.SuspendLayout();
        ((System.ComponentModel.ISupportInitialize)settingsSplit).BeginInit();
        settingsSplit.Panel1.SuspendLayout();
        settingsSplit.Panel2.SuspendLayout();
        settingsSplit.SuspendLayout();
        SuspendLayout();

        serialPanel.Dock = DockStyle.Top;
        serialPanel.Location = new Point(0, 0);
        serialPanel.Name = "serialPanel";
        serialPanel.Size = new Size(984, 75);
        serialPanel.TabIndex = 0;
        toolTip.SetToolTip(serialPanel, "COM port 연결과 수신 화면 지우기");

        settingsSplit.Dock = DockStyle.Fill;
        settingsSplit.Location = new Point(0, 0);
        settingsSplit.Name = "settingsSplit";
        settingsSplit.Panel1.Controls.Add(wifiPanel);
        settingsSplit.Panel1MinSize = 320;
        settingsSplit.Panel2.Controls.Add(measurementPanel);
        settingsSplit.Panel2MinSize = 320;
        settingsSplit.Size = new Size(984, 245);
        settingsSplit.SplitterDistance = 540;
        settingsSplit.TabIndex = 0;

        wifiPanel.Dock = DockStyle.Fill;
        wifiPanel.Name = "wifiPanel";
        wifiPanel.TabIndex = 0;
        toolTip.SetToolTip(wifiPanel, "AP/Server/DHCP 설정 Read 및 Write");
        measurementPanel.Dock = DockStyle.Fill;
        measurementPanel.Name = "measurementPanel";
        measurementPanel.TabIndex = 1;
        toolTip.SetToolTip(measurementPanel, "측정 조건 설정 Read 및 Write");

        monitorPanel.Dock = DockStyle.Fill;
        monitorPanel.Location = new Point(0, 0);
        monitorPanel.Name = "monitorPanel";
        monitorPanel.Size = new Size(984, 337);
        monitorPanel.TabIndex = 2;
        toolTip.SetToolTip(monitorPanel, "MCU 측정값 및 STATUS frame 표시");

        contentSplit.Dock = DockStyle.Fill;
        contentSplit.Location = new Point(0, 75);
        contentSplit.Name = "contentSplit";
        contentSplit.Orientation = Orientation.Horizontal;
        contentSplit.Panel1.Controls.Add(settingsSplit);
        contentSplit.Panel1MinSize = 190;
        contentSplit.Panel2.Controls.Add(monitorPanel);
        contentSplit.Panel2MinSize = 180;
        contentSplit.Size = new Size(984, 586);
        contentSplit.SplitterDistance = 245;
        contentSplit.TabIndex = 1;

        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode = AutoScaleMode.Font;
        ClientSize = new Size(984, 661);
        Controls.Add(contentSplit);
        Controls.Add(serialPanel);
        MinimumSize = new Size(900, 650);
        Name = "MainForm";
        StartPosition = FormStartPosition.CenterScreen;
        Text = "STM32 Measurement Monitor - Visual Studio 2022";
        settingsSplit.Panel1.ResumeLayout(false);
        settingsSplit.Panel2.ResumeLayout(false);
        ((System.ComponentModel.ISupportInitialize)settingsSplit).EndInit();
        settingsSplit.ResumeLayout(false);
        contentSplit.Panel1.ResumeLayout(false);
        contentSplit.Panel2.ResumeLayout(false);
        ((System.ComponentModel.ISupportInitialize)contentSplit).EndInit();
        contentSplit.ResumeLayout(false);
        ResumeLayout(false);
    }
}
