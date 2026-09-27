namespace MouseEditableTextBoxDemo;

partial class Form1
{
    private System.ComponentModel.IContainer? components = null;

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            components?.Dispose();
        }

        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        instructionLabel = new Label();
        editableTextBox = new TextBox();
        resizeGrip = new Panel();
        SuspendLayout();
        //
        // instructionLabel
        //
        instructionLabel.AutoSize = true;
        instructionLabel.Location = new Point(20, 18);
        instructionLabel.Name = "instructionLabel";
        instructionLabel.Size = new Size(499, 15);
        instructionLabel.TabIndex = 0;
        instructionLabel.Text = "텍스트 상자를 드래그하면 이동하고, 오른쪽 아래 파란 손잡이를 드래그하면 크기가 바뀝니다.";
        //
        // editableTextBox
        //
        editableTextBox.Anchor = AnchorStyles.None;
        editableTextBox.Location = new Point(120, 90);
        editableTextBox.MinimumSize = new Size(80, 40);
        editableTextBox.Multiline = true;
        editableTextBox.Name = "editableTextBox";
        editableTextBox.Size = new Size(260, 100);
        editableTextBox.TabIndex = 1;
        editableTextBox.Text = "마우스로 위치와 크기를 바꿔 보세요.";
        //
        // resizeGrip
        //
        resizeGrip.BackColor = Color.DodgerBlue;
        resizeGrip.Cursor = Cursors.SizeNWSE;
        resizeGrip.Location = new Point(368, 178);
        resizeGrip.Name = "resizeGrip";
        resizeGrip.Size = new Size(12, 12);
        resizeGrip.TabIndex = 2;
        //
        // Form1
        //
        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode = AutoScaleMode.Font;
        ClientSize = new Size(640, 360);
        Controls.Add(resizeGrip);
        Controls.Add(editableTextBox);
        Controls.Add(instructionLabel);
        MinimumSize = new Size(500, 300);
        Name = "Form1";
        StartPosition = FormStartPosition.CenterScreen;
        Text = "마우스로 조절하는 TextBox";
        ResumeLayout(false);
        PerformLayout();
    }

    private Label instructionLabel;
    private TextBox editableTextBox;
    private Panel resizeGrip;
}
