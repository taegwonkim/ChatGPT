using System.IO.Ports;

namespace MeasurementMonitor;

internal sealed class SerialPanel : GroupBox
{
    private readonly ComboBox ports = new() { DropDownStyle = ComboBoxStyle.DropDownList };
    private readonly ComboBox baud = new() { DropDownStyle = ComboBoxStyle.DropDownList };
    private readonly NumericUpDown timeout = new() { Minimum = 10, Maximum = 60000, Value = 1000 };
    private readonly Button openClose = new() { Text = "Open", AutoSize = true };
    private readonly Button refresh = new() { Text = "새로고침", AutoSize = true };
    private readonly Button clear = new() { Text = "Clear", AutoSize = true };

    internal event EventHandler? OpenCloseRequested;
    internal event EventHandler? ClearRequested;
    internal string PortName => ports.Text;
    internal int BaudRate => int.Parse(baud.Text);
    internal int Timeout => decimal.ToInt32(timeout.Value);

    internal SerialPanel()
    {
        Text = "Serial Port";
        Dock = DockStyle.Top;
        AutoSize = true;
        Padding = new Padding(10);
        baud.Items.AddRange(["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"]);
        baud.SelectedItem = "115200";
        var row = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, WrapContents = true };
        row.Controls.AddRange([new Label { Text = "COM Port", AutoSize = true, Margin = new Padding(3, 8, 3, 3) }, ports,
            refresh, new Label { Text = "Baudrate", AutoSize = true, Margin = new Padding(12, 8, 3, 3) }, baud,
            new Label { Text = "Timeout(ms)", AutoSize = true, Margin = new Padding(12, 8, 3, 3) }, timeout,
            openClose, clear]);
        Controls.Add(row);
        refresh.Click += (_, _) => RefreshPorts();
        openClose.Click += (_, _) => OpenCloseRequested?.Invoke(this, EventArgs.Empty);
        clear.Click += (_, _) => ClearRequested?.Invoke(this, EventArgs.Empty);
        RefreshPorts();
    }

    internal void SetOpen(bool isOpen)
    {
        openClose.Text = isOpen ? "Close" : "Open";
        ports.Enabled = baud.Enabled = timeout.Enabled = refresh.Enabled = !isOpen;
    }

    private void RefreshPorts()
    {
        string selected = ports.Text;
        ports.Items.Clear();
        ports.Items.AddRange(SerialPort.GetPortNames().OrderBy(x => x).ToArray());
        if (ports.Items.Contains(selected)) ports.SelectedItem = selected;
        else if (ports.Items.Count > 0) ports.SelectedIndex = 0;
    }
}
