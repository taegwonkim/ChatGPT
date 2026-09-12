using System.Text.Json;

namespace MeasurementMonitor;

internal sealed record ControlLayout(string Key, string HostKey, int X, int Y, int Width,
    int Height, string FontName, float FontSize, int FontStyle, int BackColor,
    int ForeColor, int Anchor);

internal sealed class RuntimeLayoutManager
{
    private readonly Control root;
    private readonly Dictionary<string, Control> controls = [];
    private readonly Dictionary<Control, string> keys = [];
    private readonly HashSet<Control> freed = [];
    private Panel? selectionOverlay;
    private Control? selectedControl;
    private Point dragStart;
    private Rectangle dragStartBounds;
    private bool resizing;
    private static string FilePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "STM32MeasurementMonitor", "control-layout.json");

    internal IEnumerable<KeyValuePair<string, Control>> Controls => controls;

    internal RuntimeLayoutManager(Control rootControl)
    {
        root = rootControl;
        controls["root"] = root;
        keys[root] = "root";
        Index(root, "root");
    }

    private void Index(Control parent, string parentKey)
    {
        for (int index = 0; index < parent.Controls.Count; index++)
        {
            Control child = parent.Controls[index];
            string id = string.IsNullOrWhiteSpace(child.Name) ? $"{child.GetType().Name}{index}" : child.Name;
            string key = $"{parentKey}/{id}";
            controls[key] = child;
            keys[child] = key;
            Index(child, key);
        }
    }

    internal void ApplySaved()
    {
        try
        {
            if (!File.Exists(FilePath)) return;
            ControlLayout[] layouts = JsonSerializer.Deserialize<ControlLayout[]>(File.ReadAllText(FilePath)) ?? [];
            foreach (ControlLayout layout in layouts)
            {
                if (!controls.TryGetValue(layout.Key, out Control? control) ||
                    !controls.TryGetValue(layout.HostKey, out Control? host)) continue;
                Reparent(control, host, new Rectangle(layout.X, layout.Y,
                    Math.Max(10, layout.Width), Math.Max(10, layout.Height)));
                if (layout.FontSize > 0)
                    control.Font = new Font(layout.FontName, layout.FontSize,
                        (FontStyle)layout.FontStyle, GraphicsUnit.Point);
                bool fixedMonitorValueStyle = control.Name is "status" or "macAddress";
                if (!fixedMonitorValueStyle && layout.BackColor != 0)
                    control.BackColor = Color.FromArgb(layout.BackColor);
                if (!fixedMonitorValueStyle && layout.ForeColor != 0)
                    control.ForeColor = Color.FromArgb(layout.ForeColor);
                control.Anchor = (AnchorStyles)layout.Anchor;
            }
        }
        catch (JsonException) { }
        catch (IOException) { }
        catch (ArgumentException) { }
    }

    internal void Free(Control control)
    {
        Control host = control is UserControl ? root : FindHost(control);
        Rectangle screen = control.RectangleToScreen(new Rectangle(Point.Empty, control.Size));
        Point location = host.PointToClient(screen.Location);
        Reparent(control, host, new Rectangle(location, screen.Size));
        ShowSelection(control);
    }

    internal void ShowSelection(Control control)
    {
        selectedControl = control;
        selectionOverlay?.Dispose();
        Rectangle screen = control.RectangleToScreen(new Rectangle(Point.Empty, control.Size));
        selectionOverlay = new Panel
        {
            BackColor = Color.Transparent,
            Bounds = new Rectangle(root.PointToClient(screen.Location), screen.Size),
            Cursor = Cursors.SizeAll,
            Name = "layoutSelectionOverlay"
        };
        selectionOverlay.Paint += PaintSelection;
        selectionOverlay.MouseDown += SelectionMouseDown;
        selectionOverlay.MouseMove += SelectionMouseMove;
        selectionOverlay.MouseUp += (_, _) => selectionOverlay!.Capture = false;
        root.Controls.Add(selectionOverlay);
        SetBorderRegion(selectionOverlay);
        selectionOverlay.BringToFront();
    }

    internal void ClearSelection()
    {
        selectionOverlay?.Dispose();
        selectionOverlay = null;
        selectedControl = null;
    }

    private static void SetBorderRegion(Control overlay)
    {
        var region = new Region(new Rectangle(0, 0, overlay.Width, overlay.Height));
        if (overlay.Width > 10 && overlay.Height > 10)
            region.Exclude(new Rectangle(4, 4, overlay.Width - 8, overlay.Height - 8));
        region.Union(new Rectangle(Math.Max(0, overlay.Width - 14),
            Math.Max(0, overlay.Height - 14), 14, 14));
        Region? previous = overlay.Region;
        overlay.Region = region;
        previous?.Dispose();
    }

    private static void PaintSelection(object? sender, PaintEventArgs e)
    {
        if (sender is not Control overlay) return;
        using var pen = new Pen(Color.DodgerBlue, 2F) { DashStyle = System.Drawing.Drawing2D.DashStyle.Dash };
        e.Graphics.DrawRectangle(pen, 1, 1, Math.Max(1, overlay.Width - 3), Math.Max(1, overlay.Height - 3));
        e.Graphics.FillRectangle(Brushes.DodgerBlue, Math.Max(0, overlay.Width - 12),
            Math.Max(0, overlay.Height - 12), 10, 10);
    }

    private void SelectionMouseDown(object? sender, MouseEventArgs e)
    {
        if (e.Button != MouseButtons.Left || selectedControl is null || selectionOverlay is null) return;
        if (!freed.Contains(selectedControl)) Free(selectedControl);
        dragStart = selectionOverlay.PointToScreen(e.Location);
        dragStartBounds = selectionOverlay.Bounds;
        resizing = e.X >= selectionOverlay.Width - 16 && e.Y >= selectionOverlay.Height - 16;
        selectionOverlay.Capture = true;
    }

    private void SelectionMouseMove(object? sender, MouseEventArgs e)
    {
        if (selectionOverlay?.Capture != true || selectedControl?.Parent is null) return;
        Point current = selectionOverlay.PointToScreen(e.Location);
        int dx = current.X - dragStart.X;
        int dy = current.Y - dragStart.Y;
        Rectangle bounds = dragStartBounds;
        if (resizing)
        {
            bounds.Width = Math.Max(10, bounds.Width + dx);
            bounds.Height = Math.Max(10, bounds.Height + dy);
            selectedControl.Size = bounds.Size;
            bounds.Size = selectedControl.Size; // single-line TextBox 등 실제 적용 크기를 반영
        }
        else
        {
            bounds.X += dx;
            bounds.Y += dy;
            Point screen = root.PointToScreen(bounds.Location);
            selectedControl.Location = selectedControl.Parent.PointToClient(screen);
        }
        selectionOverlay.Bounds = bounds;
        SetBorderRegion(selectionOverlay);
        selectionOverlay.Invalidate();
    }

    private Control FindHost(Control control)
    {
        for (Control? candidate = control.Parent; candidate is not null; candidate = candidate.Parent)
            if (candidate is GroupBox && keys.ContainsKey(candidate)) return candidate;
        return root;
    }

    private void Reparent(Control control, Control host, Rectangle bounds)
    {
        control.Parent?.Controls.Remove(control);
        host.Controls.Add(control);
        control.Dock = DockStyle.None;
        control.Anchor = AnchorStyles.Top | AnchorStyles.Left;
        control.Bounds = bounds;
        control.BringToFront();
        freed.Add(control);
    }

    internal bool Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
            ControlLayout[] layouts = freed.Where(control => control.Parent is not null && keys.ContainsKey(control.Parent))
                .Select(control => new ControlLayout(keys[control], keys[control.Parent!], control.Left,
                    control.Top, control.Width, control.Height, control.Font.Name, control.Font.Size,
                    (int)control.Font.Style, control.BackColor.ToArgb(), control.ForeColor.ToArgb(),
                    (int)control.Anchor)).ToArray();
            File.WriteAllText(FilePath, JsonSerializer.Serialize(layouts,
                new JsonSerializerOptions { WriteIndented = true }));
            return true;
        }
        catch (IOException) { return false; }
        catch (UnauthorizedAccessException) { return false; }
    }

    internal void Reset()
    {
        try { if (File.Exists(FilePath)) File.Delete(FilePath); }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
    }
}
