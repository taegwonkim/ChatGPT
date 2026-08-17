using System.IO.Ports;

namespace MeasurementMonitor;

public partial class SerialPanel : UserControl
{
    internal event EventHandler? OpenCloseRequested;
    internal event EventHandler? ClearRequested;
    internal string PortName => ports.Text;
    internal int BaudRate => int.Parse(baud.Text);
    internal int Timeout => decimal.ToInt32(timeout.Value);
    internal SerialSettings CurrentSettings => new(PortName, BaudRate, Timeout);

    public SerialPanel()
    {
        InitializeComponent();
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

    internal void Apply(SerialSettings value)
    {
        if (!string.IsNullOrWhiteSpace(value.PortName) && !ports.Items.Contains(value.PortName))
            ports.Items.Add(value.PortName);
        if (ports.Items.Contains(value.PortName)) ports.SelectedItem = value.PortName;

        if (value.BaudRate > 0)
        {
            string savedBaud = value.BaudRate.ToString();
            if (!baud.Items.Contains(savedBaud)) baud.Items.Add(savedBaud);
            baud.SelectedItem = savedBaud;
        }
        timeout.Value = Math.Clamp(value.Timeout, decimal.ToInt32(timeout.Minimum),
            decimal.ToInt32(timeout.Maximum));
    }

    private void RefreshPorts()
    {
        string selected = ports.Text;
        ports.Items.Clear();
        ports.Items.AddRange(SerialPort.GetPortNames().OrderBy(x => x).ToArray());
        if (!string.IsNullOrWhiteSpace(selected) && !ports.Items.Contains(selected))
            ports.Items.Add(selected);
        if (ports.Items.Contains(selected)) ports.SelectedItem = selected;
        else if (ports.Items.Count > 0) ports.SelectedIndex = 0;
    }
}
