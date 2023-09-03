using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using System.IO.Ports;
using System.Text;
using static StorageManager.Program;

namespace StorageManager
{
    internal class Program
    {
        public static AutoResetEvent continueFileUploadEvent = new AutoResetEvent(false);
        static int wholeThingChecksum = 0;

        const int DOOM_MAX_FILE_LENGTH = 4300800;

        static void Main(string[] args)
        {
            Console.WriteLine("Available COM ports: ");
            foreach (string portx in SerialPort.GetPortNames())
            {
                Console.WriteLine("\t- " + portx);
            }

            Console.Write("Enter the COM port to use: ");
            string? portName = Console.ReadLine();
            SerialPort port = new SerialPort(portName, 115200);
            port.Open();
            Task.Run(() => {
                while (true)
                {
                    FileUploadParseData(port.ReadLine());
                }
            });

            while (true)
            {
                Console.WriteLine($"Select storage type to upload:{Environment.NewLine}\t1) Doom WAD{Environment.NewLine}\t2) Everything else");
                Console.Write("Enter the option number: ");
                switch (Console.ReadLine())
                {
                    case "1":
                        Console.WriteLine("The first file with the extension .wad will be used. Press any key to continue...");
                        string? filePath = Directory.GetFiles(Directory.GetCurrentDirectory(), "*.wad", SearchOption.AllDirectories).OrderBy(p=>new FileInfo(p).Name).FirstOrDefault();

                        if (filePath != null)
                        {
                            var toUpload = File.ReadAllBytes(filePath);
                            if(toUpload.Length> DOOM_MAX_FILE_LENGTH - 4) //-4 beause we are including the file size
                            {
                                Console.WriteLine($"Doom file size is larger than supported! ({DOOM_MAX_FILE_LENGTH} bytes)");
                                    continue;
                            }
                            UploadData(port, 0, File.ReadAllBytes(filePath),true);
                        }
                        else
                        {
                            Console.WriteLine("Could not find any *.wad files in " + Directory.GetCurrentDirectory());
                        }                        
                        break;
                    case "2":
                        CreateAndUploadEmulatorPack(port);
                        break;
                    default:
                        Console.WriteLine("Unknown input. Please try again");
                        break;
                }
            }
        }

        private static void CreateAndUploadEmulatorPack(SerialPort port)
        {
            Console.WriteLine("Files with .gbc, .gb, .bios, .png222, .png1 extensions in the current folder will be packed.");
            Console.Write("Do you want to ONLY pack images? (y/n): ");
            bool onlyImages = Console.ReadLine()?.ToLower() == "y";
            var fileList = Directory.GetFiles(Environment.CurrentDirectory,"*", SearchOption.AllDirectories).ToList();
            fileList = fileList.OrderBy(p => new FileInfo(p).Name).ToList();

            byte[] toSend=new byte[0];
            int totalFileCount = 0;

            using (MemoryStream ms = new MemoryStream())
            {
                using(BinaryWriter sw = new BinaryWriter(ms))
                {
                    foreach(var file in fileList)
                    {
                        FileInfo fileInfo = new FileInfo(file);
                        var fileType = GetFileTypeFromExtension(fileInfo.Extension.ToLower().Replace(".",""));
                        if (fileType == StorageFileType.Unknown) continue;     
                        if(onlyImages && fileType!=StorageFileType.RGBA2221Image) continue; //Skip non-images if onlyImages is true
                                                
                        totalFileCount++;

                        short param1ToWrite = 0;
                        short param2ToWrite = 0;

                        string fileName = fileInfo.Name;
                        List<byte> fileData = new List<byte>();
                        switch (fileType)
                        {
                            case StorageFileType.RGBA2221Image:
                                fileData = ConvertImageToRGBA2221(Image.Load<Rgba32>(fileInfo.FullName), ref param1ToWrite, ref param2ToWrite).ToList();
                                break;
                            default:
                                fileData = File.ReadAllBytes(file).ToList();
                                break;
                        }
                                             


                        sw.Write((byte)fileType); //1 byte
                        sw.Write((byte)0x69); //Magic number
                        sw.Write((short)param1ToWrite); //2bytes
                        sw.Write((short)param2ToWrite); //2bytes
               
                        fileName=new string(fileName.Split('.')[0].Take(19).ToArray()) ?? "N/A"; //Remove the extension and set max length
                        sw.Write(UTF8Encoding.ASCII.GetBytes(fileName)); //
                        sw.Write(new byte[20 - fileName.Length]); //At least 1 null byte of padding for the name

                                 

                        //Always align to a 4 byte boundary
                        int alginmentBits = fileData.Count & 0x03;
                        int extraAlignmentNeeded = (4 - alginmentBits) % 4;
                        fileData.AddRange(new byte[extraAlignmentNeeded]);

                        Console.WriteLine($"Adding file [{fileType}] {fileInfo.Name} ({fileData.Count/1024}KB)");

                        sw.Write((UInt32)fileData.Count); //4 bytes
                        sw.Write(fileData.ToArray());
                    }

                    toSend = ms.ToArray();
                }
            }

            Console.Write($"A total of {toSend.Length / 1024}KB in {totalFileCount} files will be written. Press enter to continue...");
            Console.ReadLine();

            UploadData(port, DOOM_MAX_FILE_LENGTH, toSend, false);
        }

        private static StorageFileType GetFileTypeFromExtension(string extension)
        {
            switch (extension)
            {
                case "gb":
                    return StorageFileType.GB;
                case "gbc":
                    return StorageFileType.GBC;
                case "bios":
                    return StorageFileType.BIOS;
                case "png2221":
                    return StorageFileType.RGBA2221Image;
                case "gba":
                    return StorageFileType.GBA;
                default:
                    return StorageFileType.Unknown;
            }
        }

        [Flags]
        public enum StorageFileType:byte
        {
            GB = 1,
            GBC = 2,
            BIOS = 4,
            RGBA2221Image = 8,
            GBA = 16,
            Unknown = 128,
        }

        private static void FileUploadParseData(string line)
        {
            if (line.Contains("UPLOAD|OK"))
            {
                continueFileUploadEvent.Set();
            }
            else
            {
                if (line.Contains("UPLOAD|CHK|"))
                {
                    uint receivedChecksum = uint.Parse(line.Split('|').Last());
                    if (receivedChecksum == wholeThingChecksum)
                    {
                        Console.WriteLine("Final checksum OK!");
                    }
                    else
                    {
                        Console.WriteLine("Final checksum wrong!");
                    }
                    continueFileUploadEvent.Set();
                }
            }
        }

        public static void UploadData(SerialPort port, int offset, byte[] data, bool includeSize)
        {
            if (offset % 4096 != 0) throw new Exception("offset needs to be a multiple of 4096!");

            Console.WriteLine("Starting new data upload...");

            byte[] fileDataRAW = data;
            int totalLength = fileDataRAW.Length;

            byte[] fileData;

            if (includeSize)
            {
                fileData = new byte[fileDataRAW.Length + 4]; //Add space for the size of the file at the beggining
                BitConverter.GetBytes(totalLength).CopyTo(fileData, 0);
                fileDataRAW.CopyTo(fileData, 4);
            }
            else
            {
                fileData = fileDataRAW;
            }


            wholeThingChecksum = 0;
            for (int i = 0; i < fileData.Length; i++) wholeThingChecksum += fileData[i];
            int bytesSent = 0;
            continueFileUploadEvent.Reset();
            
            port.Write(new byte[] { 0x69 }, 0, 1);
            port.Write(BitConverter.GetBytes((Int32)offset), 0, 4);

            Console.WriteLine(); //To erase the progress later on
            do
            {
                int toSend = Math.Min(fileData.Length - bytesSent, 4096);


                Console.Write("\rSending chunk with size " + toSend + $" ({bytesSent}/{fileData.Length}) ({(int)Math.Round(100 * ((float)bytesSent / fileData.Length))}%)...             ");
                //progressBar1.Value = (int)Math.Round(100 * ((float)bytesSent / fileInfo.Length));
 

                port.Write(BitConverter.GetBytes((Int32)toSend), 0, 4);
                /*for(int i = 0; i < toSend; i++)
                {
                    port.Write(new byte[1] { fileInfo[i] },0,1);
                    //Thread.Sleep(1);
                    Console.WriteLine(i);
                }*/
                port.Write(fileData, bytesSent, toSend);
                int checksum = 0;
                for (int i = bytesSent; i < bytesSent + toSend; i++) checksum += fileData[i];
                port.Write(BitConverter.GetBytes((UInt32)checksum), 0, 4);
                port.BaseStream.Flush();

                bytesSent += toSend;
                if (toSend == 0) break; //Exit after sending a 0 length chunk
            } while (continueFileUploadEvent.WaitOne(TimeSpan.FromSeconds(10)));
            Console.WriteLine(); //endline for the progress animation

            if (!continueFileUploadEvent.WaitOne(TimeSpan.FromSeconds(10))){
                Console.WriteLine("Failed to receive final checksum in time!");
            }
            else
            {
                Console.WriteLine("File upload finished");
            }
        }

        public static byte[] ConvertImageToRGBA2221(Image<Rgba32> input, ref short param1ToWrite, ref short param2ToWrite)
        {
            param1ToWrite = (short)input.Width;
            param2ToWrite = (short)input.Height;

            using(MemoryStream ms = new MemoryStream())
            {
                for(int y= 0; y < input.Height; y++)
                {
                    for(int x=0; x < input.Width; x++)
                    {
                        //Get color
                        var color = input[x, y];
                        ms.WriteByte((byte)((byte)(color.R & 0b11000000) | (byte)((color.G & 0b11000000) >> 2) | (byte)((color.B & 0b11000000) >> 4) | (byte)((color.A & 0b11000000) >> 6))); //rrrggbbb                            
                    }
                }
                return ms.ToArray();
            }
        }
    }
}