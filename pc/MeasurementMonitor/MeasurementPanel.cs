namespace MeasurementMonitor;

public partial class MeasurementPanel : UserControl
{
    internal event EventHandler? ReadRequested;
    internal event EventHandler<MeasurementSettings>? WriteRequested;

    public MeasurementPanel()
    {
        InitializeComponent();
        readButton.Click += (_, _) => ReadRequested?.Invoke(this, EventArgs.Empty);
        writeButton.Click += (_, _) => WriteRequested?.Invoke(this,
            new(reference.Value, offset.Value, resistance.Value, interval.Value));
    }

    internal void Apply(MeasurementSettings value)
    {
        reference.Value = Clamp(reference, value.ReferenceMv);
        offset.Value = Clamp(offset, value.OffsetMv);
        resistance.Value = Clamp(resistance, value.ResistanceMilliOhm);
        interval.Value = Clamp(interval, value.IntervalSeconds);
    }
    internal MeasurementSettings CurrentSettings => new(reference.Value, offset.Value,
        resistance.Value, interval.Value);

    private static decimal Clamp(NumericUpDown control, decimal value) =>
        Math.Min(control.Maximum, Math.Max(control.Minimum, value));

}
