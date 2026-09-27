namespace MouseEditableTextBoxDemo;

public partial class Form1 : Form
{
    private const int GripSize = 12;
    private Point dragStartScreen;
    private Rectangle startBounds;
    private bool moving;
    private bool resizing;

    public Form1()
    {
        InitializeComponent();

        editableTextBox.MouseDown += EditableTextBox_MouseDown;
        editableTextBox.MouseMove += EditableTextBox_MouseMove;
        editableTextBox.MouseUp += EditableTextBox_MouseUp;
        editableTextBox.Move += (_, _) => PositionResizeGrip();
        editableTextBox.Resize += (_, _) => PositionResizeGrip();

        resizeGrip.MouseDown += ResizeGrip_MouseDown;
        resizeGrip.MouseMove += ResizeGrip_MouseMove;
        resizeGrip.MouseUp += ResizeGrip_MouseUp;

        PositionResizeGrip();
    }

    private void EditableTextBox_MouseDown(object? sender, MouseEventArgs e)
    {
        if (e.Button != MouseButtons.Left)
        {
            return;
        }

        moving = true;
        dragStartScreen = Cursor.Position;
        startBounds = editableTextBox.Bounds;
        editableTextBox.Capture = true;
    }

    private void EditableTextBox_MouseMove(object? sender, MouseEventArgs e)
    {
        if (!moving)
        {
            return;
        }

        Point delta = Subtract(Cursor.Position, dragStartScreen);
        int maxX = Math.Max(0, ClientSize.Width - startBounds.Width);
        int maxY = Math.Max(0, ClientSize.Height - startBounds.Height);
        editableTextBox.Location = new Point(
            Math.Clamp(startBounds.X + delta.X, 0, maxX),
            Math.Clamp(startBounds.Y + delta.Y, 0, maxY));
    }

    private void EditableTextBox_MouseUp(object? sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Left)
        {
            moving = false;
            editableTextBox.Capture = false;
        }
    }

    private void ResizeGrip_MouseDown(object? sender, MouseEventArgs e)
    {
        if (e.Button != MouseButtons.Left)
        {
            return;
        }

        resizing = true;
        dragStartScreen = Cursor.Position;
        startBounds = editableTextBox.Bounds;
        resizeGrip.Capture = true;
    }

    private void ResizeGrip_MouseMove(object? sender, MouseEventArgs e)
    {
        if (!resizing)
        {
            return;
        }

        Point delta = Subtract(Cursor.Position, dragStartScreen);
        int maxWidth = ClientSize.Width - startBounds.Left;
        int maxHeight = ClientSize.Height - startBounds.Top;
        editableTextBox.Size = new Size(
            Math.Clamp(startBounds.Width + delta.X, editableTextBox.MinimumSize.Width, maxWidth),
            Math.Clamp(startBounds.Height + delta.Y, editableTextBox.MinimumSize.Height, maxHeight));
    }

    private void ResizeGrip_MouseUp(object? sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Left)
        {
            resizing = false;
            resizeGrip.Capture = false;
        }
    }

    private void PositionResizeGrip()
    {
        resizeGrip.Bounds = new Rectangle(
            editableTextBox.Right - GripSize,
            editableTextBox.Bottom - GripSize,
            GripSize,
            GripSize);
        resizeGrip.BringToFront();
    }

    private static Point Subtract(Point current, Point origin) =>
        new(current.X - origin.X, current.Y - origin.Y);
}
