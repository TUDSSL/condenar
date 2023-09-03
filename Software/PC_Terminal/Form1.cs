using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.Automation;

namespace DoomTests
{
    public partial class Form1 : Form
    {
        Bitmap originalBitmap = null;
        Bitmap finalBitmap = null;
        
        public Form1()
        {
            InitializeComponent();

            var color1 = ColorFromRGB(128, 0, 0);
            var colorBack = GetColorFromShort(color1);
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        byte[] dataBytes = null;

        private void buttonRefresh_Click(object sender, EventArgs e)
        {
            if (dataBytes == null)
            {
                string data = File.ReadAllText("frame.txt");
                dataBytes = StringToByteArray(data);
            }
            
            short[] dataShort = new short[dataBytes.Length / 2];
            Buffer.BlockCopy(dataBytes, 0, dataShort, 0, dataBytes.Length);

            byte[] bwArray = new byte[dataShort.Length];

            if (originalBitmap != null) originalBitmap.Dispose();
            if (finalBitmap != null) finalBitmap.Dispose();
            originalBitmap = new Bitmap(240, 240);
            finalBitmap = new Bitmap(240, 240);
            for (int i = 0; i < 240; i++)
            {
                for (int j = 0; j < 240; j++)
                {
                    int index = i * 240 + j;
                    short value = dataShort[index];

                    var colora = GetColorFromShort(value);
                    Color color = Color.FromArgb(colora.r, colora.g, colora.b);
                    originalBitmap.SetPixel(j, i, color);

                    float floatVal = ((0.21f * color.R) + (0.71f * color.G) + (0.072f * color.B));
                    floatVal = ((0.25f * color.R) + (0.5f * color.G) + (0.25f * color.B));
                    byte bw = (byte)floatVal;
                    bwArray[index] = bw;
                }
            }

            if (radioButtonThresholding.Checked)
            {
                DoThresholding(bwArray, finalBitmap);
            }else if (radioButtonDitheting.Checked)
            {
                DoDithering(bwArray, finalBitmap);
            }else if (radioButtonBW.Checked)
            {
                DoBW(bwArray, finalBitmap);
            }

            pictureBoxOriginal.Image = originalBitmap;
            pictureBoxOriginal.Image.Save("test.png");
            pictureBoxFinal.Image = finalBitmap;
        }

        private void DoDithering(byte[] bwArray, Bitmap finalBitmap)
        {
            int[] tempBuffer = new int[bwArray.Length];
            for (int i = 0; i < bwArray.Length; i++) tempBuffer[i] = bwArray[i];

            for (int row = 0; row < finalBitmap.Height; row++)
            {
                for (int column = 0; column < finalBitmap.Width; column++)
                {
                    int oldPixel = tempBuffer[GetIndex(row, column, finalBitmap.Width)];
                    int newPixel = oldPixel > (float)numericUpDown1.Value ? (byte)255 : (byte)0;
                    int error = oldPixel - newPixel;

                    tempBuffer[GetIndex(row, column, finalBitmap.Width)] = newPixel;
                    if(column+1<finalBitmap.Width) tempBuffer[GetIndex(row, column + 1, finalBitmap.Width)] += (int)(error * 7.0f / 16.0f);
                    if (row + 1 < finalBitmap.Height && column-1 >=0) tempBuffer[GetIndex(row +1, column - 1, finalBitmap.Width)] += (int)(error * 3.0f / 16.0f);
                    if (row + 1 < finalBitmap.Height) tempBuffer[GetIndex(row + 1, column, finalBitmap.Width)] += (int)(error * 5.0f / 16.0f);
                    if (row + 1 < finalBitmap.Height && column + 1 < finalBitmap.Width)  tempBuffer[GetIndex(row + 1, column +1, finalBitmap.Width)] += (int)(error * 1.0f / 16.0f);

                }
            }

            for (int i = 0; i < 240; i++)
            {
                for (int j = 0; j < 240; j++)
                {
                    int index = i * 240 + j;
                    byte value = (byte)tempBuffer[index];
                    Color color = Color.FromArgb(value, value, value);
                    finalBitmap.SetPixel(j, i, color);
                }
            }
        }

        private int GetIndex(int row, int column, int width)
        {            
            return row * width + column;
        }

        private void DoBW(byte[] bwArray, Bitmap finalBitmap)
        {
            for (int row = 0; row < finalBitmap.Height; row++)
            {
                for (int column = 0; column < finalBitmap.Width; column++)
                {
                    var value = bwArray[row * finalBitmap.Width + column];
                    finalBitmap.SetPixel(column, row,  Color.FromArgb(255,value,value,value));
                }
            }
        }

        private void DoThresholding(byte[] bwArray, Bitmap finalBitmap)
        {
            for(int row = 0; row < finalBitmap.Height; row++)
            {
                for(int column = 0; column < finalBitmap.Width; column++)
                {
                    finalBitmap.SetPixel(column, row, bwArray[row * finalBitmap.Width + column] > numericUpDown1.Value ? Color.White : Color.Black);
                }
            }
        }

        public (byte r,byte g,byte b) GetColorFromShort(short input)
        {
            short value = input;
            value = IPAddress.NetworkToHostOrder(value);
            int red = ((value & 0b11111000) >> 3);
            int green = ((value & 0b00000111) << 3) | ((value & 0b1110000000000000) >> 13);
            int blue = ((value & 0b0001111100000000) >> 8);
            red = red * 8;
            green = green * 4;
            blue = blue * 8;
            return ((byte)red, (byte)green, (byte)blue);
        }

        public short ColorFromRGB(byte r, byte g, byte b)
        {
            return (short)((((r) & 0xF8) | (((g) & 0x1C) << 11) | (((g) & 0xE0) >> 5) | (((b) & 0x1F) << 8)));
        }

        public static byte[] StringToByteArray(String hex)
        {
            int NumberChars = hex.Length;
            byte[] bytes = new byte[NumberChars / 2];
            for (int i = 0; i < NumberChars; i += 2)
                bytes[i / 2] = Convert.ToByte(hex.Substring(i, 2), 16);
            return bytes;
        }

        private void radioButtonThresholding_CheckedChanged(object sender, EventArgs e)
        {
            buttonRefresh.PerformClick();
        }

        private void buttonConnectRTT_Click(object sender, EventArgs e)
        {
            Task.Run(() => {

              
                while (true)
                {
                    try
                    {
                        Console.WriteLine("Trying to connect to RTT...");
                        
                        TcpClient tcpClient = new TcpClient();
                        tcpClient.Connect(IPAddress.Parse(textBox2.Text), 19021);

                      //  int frameLenghtBytes = 115200;
                        int frameLenghtBytes = 9600;

                        var stream = tcpClient.GetStream();
                        BinaryReader binaryReader = new BinaryReader(stream);

                        bool inSync = false;
                        byte[] outOfSyncBuffer;
                        byte[] mainBuffer = new byte[1024 * 1024];
                        int mainBufferReadByteCount = 0;
                        byte[] header = { 0x69, 0x69, 0x06, 0x09, 0x19, 0x13, 0x69, 0x68 };



                        while (tcpClient.Connected)
                        {
                            if (inSync)
                            {
                                mainBufferReadByteCount = 0;
                                while (mainBufferReadByteCount < frameLenghtBytes + header.Length)
                                {
                                    int justRead = binaryReader.Read(mainBuffer, mainBufferReadByteCount, frameLenghtBytes + header.Length - mainBufferReadByteCount);
                                    if (justRead == 0)
                                    {
                                        tcpClient.Dispose();
                                        throw new Exception("Connection closed");
                                    }
                                    mainBufferReadByteCount += justRead;
                                }
                                inSync = mainBuffer.Take(header.Length).SequenceEqual(header);

                                if (inSync)
                                {
                                    Console.WriteLine("Frame ready!");
                                    /* dataBytes = mainBuffer.Skip(header.Length).Take(frameLenghtBytes).ToArray();
                                     this.Invoke((MethodInvoker)delegate () {
                                         buttonRefresh.PerformClick();
                                     });*/

                                    var dataToUse= mainBuffer.Skip(header.Length).Take(frameLenghtBytes).ToArray();
                                    
                               
                                    Bitmap finalBitmapx = new Bitmap(320, 240, PixelFormat.Format24bppRgb);
                                    for(int row = 0; row < 240; row++)
                                    {
                                        for(int column = 0; column < 320; column++)
                                        {                                            
                                            int index = row * 320 + column;
                                            int byteIndex = index / 8;
                                            int bitIndex = 7 - (index % 8);

                                            var isWhite = (dataToUse[byteIndex] & (1 << bitIndex)) > 0;                                           
                                            finalBitmapx.SetPixel(column, row,isWhite?Color.White:Color.Black);
                                        }                                    
                                    }
                                    this.Invoke((MethodInvoker)delegate () {
                                        pictureBoxFinal.Image = finalBitmapx;
                                    }); 
                                }
                                else
                                {
                                    Console.WriteLine("Out of sync!");
                                }
                            }
                            else
                            {
                                outOfSyncBuffer = binaryReader.ReadBytes(1024);
                                if (outOfSyncBuffer.Length == 0)
                                {                                    
                                    tcpClient.Dispose();
                                    throw new Exception("Connection closed");                                    
                                }
                                int foundIndex = StartingIndex(outOfSyncBuffer, header);
                                if (foundIndex != -1)
                                {
                                    Console.WriteLine("Sync header found!");
                                    int validBytesRead = outOfSyncBuffer.Length - foundIndex - header.Length;
                                    binaryReader.ReadBytes(frameLenghtBytes - validBytesRead);
                                    inSync = true;
                                }
                            }

                        }
                    }
                    catch (Exception ex)
                    {
                        Thread.Sleep(1000);
                    }
                }
                              
            });
        }



        //From https://stackoverflow.com/questions/1780423/find-the-first-occurrence-starting-index-of-the-sub-array-in-c-sharp
        private static bool IsSubArrayEqual(byte[] x, byte[] y, int start)
        {
            for (int i = 0; i < y.Length; i++)
            {
                if (x[start++] != y[i]) return false;
            }
            return true;
        }
        public static int StartingIndex(byte[] x, byte[] y)
        {
            int max = 1 + x.Length - y.Length;
            for (int i = 0; i < max; i++)
            {
                if (IsSubArrayEqual(x, y, i)) return i;
            }
            return -1;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            byte[,] output;

            using (BinaryReader reader = new BinaryReader(File.OpenRead("STBAR.lmp")))
            {
                short width = reader.ReadInt16();
                short height = reader.ReadInt16();
                short left = reader.ReadInt16();
                short top = reader.ReadInt16();

                output = new byte[height, width];

                int[] offsets = new int[width];
                for (int i = 0; i < width; i++) offsets[i] = reader.ReadInt32();

                for(int i = 0; i < width; i++)
                {
                    reader.BaseStream.Position = offsets[i];
                    int rowstart = 0;
                    while (rowstart != 255)
                    {
                        rowstart = reader.ReadByte();
                        if (rowstart == 255) break;

                        int pixelCount = reader.ReadByte();
                        reader.ReadByte();

                        for(int j = 0; j < pixelCount; j++)
                        {
                            byte color = reader.ReadByte();
                            output[j+rowstart, i] = color;
                        }

                        reader.ReadByte();
                    }
                }

                string outputString = "";
                for (int i = 0; i < height; i++)
                    for (int j = 0; j < width; j++)
                {
                   
                    {
                    
                        outputString += output[i, j] + ", ";
                    }
                }
                Console.WriteLine(outputString);
            }


            using (BinaryReader reader = new BinaryReader(File.OpenRead("STARMS.lmp")))
            {
                short width = reader.ReadInt16();
                short height = reader.ReadInt16();
                short left = reader.ReadInt16();
                short top = reader.ReadInt16();

                //output = new byte[height, width];

                int[] offsets = new int[width];
                for (int i = 0; i < width; i++) offsets[i] = reader.ReadInt32();

                for (int i = 0; i < width; i++)
                {
                    reader.BaseStream.Position = offsets[i];
                    int rowstart = 0;
                    while (rowstart != 255)
                    {
                        rowstart = reader.ReadByte();
                        if (rowstart == 255) break;

                        int pixelCount = reader.ReadByte();
                        reader.ReadByte();

                        for (int j = 0; j < pixelCount; j++)
                        {
                            byte color = reader.ReadByte();
                            output[j + rowstart, i+104] = color;
                        }

                        reader.ReadByte();
                    }
                }

                string outputString = "";
                for (int i = 0; i < height; i++)
                    for (int j = 0; j < width; j++)
                    {

                        {

                            outputString += output[i, j] + ", ";
                        }
                    }
                Console.WriteLine(outputString);
            }


            Task.Run(() => {
                try
                {
                    // Start the child process.
                    Process p = new Process();
                    p.StartInfo.UseShellExecute = false;
                    p.StartInfo.RedirectStandardOutput = true;
                    p.StartInfo.FileName = "cmd.exe";
                    p.StartInfo.CreateNoWindow = true;
                    p.StartInfo.WorkingDirectory = "C:\\";
                    p.StartInfo.Arguments = "/c wsl hostname -I";
                    p.Start();
                    string output2 = p.StandardOutput.ReadToEnd();
                    p.WaitForExit();
                    this.Invoke((MethodInvoker)delegate ()
                    {
                        textBox2.Text = output2.Split('\n').First(q=>q.Contains(".")).Replace("\n","").Replace("\r","").Trim();
                    });
                }
                catch (Exception)
                {

                    throw;
                }
            });
        }

        private void button1_Click(object sender, EventArgs e)
        {
            List<string> lines = File.ReadAllLines("frameAlt.txt").ToList();
            while (lines[0].StartsWith("[")) lines.RemoveAt(0);
            if (lines.Last().StartsWith("[")) lines.RemoveAt(lines.Count-1);

            if (originalBitmap != null) originalBitmap.Dispose();
          
            originalBitmap = new Bitmap(240, 240);

            for (int i = 0; i < 240; i++)
            {
                var pixels = lines[i].Split(' ').Where(p=>!String.IsNullOrWhiteSpace(p)).ToList();
                for (int j = 0; j < 240; j++)
                {
                    var stringData = pixels[j];
                    int index = i * 240 + j;

                    //From hex
                    var color = Color.FromArgb(Convert.ToByte(stringData.Substring(0, 2), 16), Convert.ToByte(stringData.Substring(2, 2), 16), Convert.ToByte(stringData.Substring(4, 2), 16));
                
                    originalBitmap.SetPixel(j, i, color);
                }
            }                       

            pictureBoxOriginal.Image = originalBitmap;
        }
    }
        


    
}
