namespace MeasurementMonitor;

public partial class MeasurementPanel : UserControl
{
    internal event EventHandler? ReadRequested;
    internal event EventHandler<MeasurementSettings>? WriteRequested;
    internal event EventHandler? ResetReadRequested;
    internal event EventHandler<uint>? ResetWriteRequested;

    public MeasurementPanel()
    {
        InitializeComponent();
        readButton.Click += (_, _) => ReadRequested?.Invoke(this, EventArgs.Empty);
        writeButton.Click += (_, _) => WriteRequested?.Invoke(this,
            new(reference.Value, offset.Value, resistance.Value, interval.Value,
                rs485Only.SelectedIndex == 1));
        resetReadButton.Click += (_, _) => ResetReadRequested?.Invoke(this, EventArgs.Empty);
        resetWriteButton.Click += (_, _) => ResetWriteRequested?.Invoke(this, (uint)resetInterval.Value);
    }

    internal void Apply(MeasurementSettings value)
    {
        SetValueWithinRange(reference, value.ReferenceMv);
        SetValueWithinRange(offset, value.OffsetMv);
        SetValueWithinRange(resistance, value.ResistanceMilliOhm);
        SetValueWithinRange(interval, value.IntervalSeconds);
        rs485Only.SelectedIndex = value.Rs485Only ? 1 : 0;
    }
    internal MeasurementSettings CurrentSettings => new(reference.Value, offset.Value,
        resistance.Value, interval.Value, rs485Only.SelectedIndex == 1);
    internal uint ResetIntervalSeconds => (uint)resetInterval.Value;
    internal void ApplyResetInterval(uint seconds) =>
        SetValueWithinRange(resetInterval, seconds);

    private static void SetValueWithinRange(NumericUpDown control, decimal requestedValue)
    {
        if (control.IsDisposed || control.Disposing) return;

        // Minimum/Maximum을 먼저 snapshot한 뒤 범위를 제한합니다. 저장 파일이나 MCU
        // 응답값이 UI 범위를 벗어나도 NumericUpDown.Value에는 유효한 값만 전달됩니다.
        decimal minimum = control.Minimum;
        decimal maximum = control.Maximum;
        decimal boundedValue = Math.Min(maximum, Math.Max(minimum, requestedValue));
        if (control.Value != boundedValue)
            control.Value = boundedValue;
    }

}
