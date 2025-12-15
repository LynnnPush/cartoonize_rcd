// Streamulator test platform

#include "streamulator.h"
#include "../../cartoonize_pipeline.h"   // replace with desired processing module


/* Load image from file into pixel stream
 *
 * filename - path to input image
 * stream   - output pixel stream
 * rep      - number of times to repeat the image
 */
void loadFrame (const std::string &filename, pixel_stream &stream, int rep)
{
	// Read input image
	unsigned char *idata = NULL;
	struct spng_ihdr ihdr;
	if (pngread(filename, &idata, &ihdr)) {
		std::cout << "##### Unable to read image: " << filename << std::endl;
		throw;
	}

	// Write input data
	for (int i=0; i<rep; i++)
		img2axi(idata, ihdr, stream);

	free(idata);
}


/* Generate frame with a single solid color
 * color  - color in BGR format
 * stream - output pixel stream
 * rep    - number of times to repeat the image
 */
void fillFrame (int color, pixel_stream &stream, int rep)
{
	pixel_data pixel;

	for (int row=0; row<HEIGHT; row++)
		for (int col=0; col<WIDTH; col++)
		{
			pixel.user = (row == 0 && col == 0) ? 1 : 0;
			pixel.last = (col == WIDTH-1) ? 1 : 0;

			pixel.data = color;
			stream << pixel;
		}
}


/* Process image stream
 *
 * src - source (input) stream
 * dst - destination (output) stream
 */
void processStream (pixel_stream &src, pixel_stream &dst)
{
	static int pixel = 0;
	static int frame = 0;

	// Call stream processing function
	while (!src.empty())
	{
		stream(src, dst, frame);

		pixel++;
		if (pixel >= WIDTH*HEIGHT)
		{
			pixel = 0;
			frame++;
			//std::cout << "Frame: " << frame << std::endl; // uncomment to see frame progress
		}
	}
}


/* Save raw pixel stream to file
 *
 * src      - input pixel stream
 * filename - path to output image
 */
void saveRawStream (pixel_stream &src, const std::string &filename)
{
	std::vector<unsigned char> pixeldata;
	pixel_data pixel;

	// Copy source data
	while(src.read_nb(pixel))
	{
		pixeldata.push_back((pixel.data      ) & 0xFF);
		pixeldata.push_back((pixel.data >>  8) & 0xFF);
		pixeldata.push_back((pixel.data >> 16) & 0xFF);
	}

	// Compute height
	int height = pixeldata.size() / 3 / WIDTH;

	// Save image
	struct spng_ihdr ihdr = {0};
	ihdr.width = WIDTH;
	ihdr.height = height;
	ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
	ihdr.bit_depth = 8;
	pngwrite(filename, pixeldata.data(), &ihdr);
}


/* Synchronize video stream
 *
 * src - input pixel stream
 * dst - output pixel stream
 *
 * returns number of pixels before sync signal (ie processing delay)
 */
int syncStream (pixel_stream &src, pixel_stream &dst)
{
	pixel_data pixel;

	// Can't sync empty stream
	if (src.empty())
		return -1;

	// Find start of frame
	int i = -1;
	do
	{
		if (!src.read_nb(pixel))
		{
			std::cout << "##### Unable to sync frame, maybe missing a pixel.user assignment?" << std::endl;
			throw;
		}
		i++;
	} while (pixel.user != 1);

	// Copy rest of stream
	dst << pixel;
	while (src.read_nb(pixel))
		dst << pixel;

	return i;
}


/* Save a single frame from pixel stream
 *
 * src        - input pixel stream
 * filename   - path to output image
 */
void saveFrame (pixel_stream &src, const std::string &filename)
{
	unsigned char *idata = new unsigned char[WIDTH*HEIGHT*3];
	pixel_data pixel;

	// Read frame, keep track of lines missing pixel.last signal
	int missing_last = 0;
	int row;

	for (row = 0; row < HEIGHT; row++)
		for (int col = 0; col < WIDTH; col++)
		{
			if (!src.read_nb(pixel))
				goto endloop;

			unsigned char *pdata = &idata[(row * WIDTH + col) * 3];
			pdata[0] = (pixel.data      ) & 0xFF;
			pdata[1] = (pixel.data >>  8) & 0xFF;
			pdata[2] = (pixel.data >> 16) & 0xFF;

			if (col == WIDTH-1 && pixel.last != 1) // check pixel.last
				missing_last++;


		}
endloop:

	if (missing_last > 0)
		std::cout << "Number of lines missing pixel.last signal: " << missing_last << std::endl;

	// Save image
	if (!filename.empty())
	{
		struct spng_ihdr ihdr = {0};
		ihdr.width = WIDTH;
		ihdr.height = HEIGHT;
		ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
		ihdr.bit_depth = 8;
		pngwrite(filename, idata, &ihdr);
	}

	delete[] idata;
}


int main ()
{
	pixel_stream srcStream, procStream, syncedStream;

	std::ifstream frames_list(FRAMES_LIST);
	std::string filename;
	int reps, delay = -1, frame = 0;

	while (frames_list >> filename && frames_list >> reps)
	{
		std::cout << filename << " " << reps << std::endl;
		if (filename == "-")
			fillFrame(0, srcStream, reps);
		else
			loadFrame(INPUT_DIR + filename, srcStream, reps);

		processStream(srcStream, procStream);

#ifndef RAW_FILENAME
		delay = syncStream(procStream, syncedStream);

		for (int i=0; i<reps; i++)
		{
			std::ostringstream os;
			os << OUTPUT_DIR;
			os << std::setfill('0') << std::setw(4) << frame;
			os << "_" << filename;

			if (filename == "-")
				saveFrame(syncedStream, "");
			else
				saveFrame(syncedStream, os.str());
			frame++;
		}
#endif
	}
#ifdef RAW_FILENAME
	saveRawStream(procStream, RAW_FILENAME);
#endif

	std::cout << "HLS delay: " << delay << " pixels"
			  << " (" << (float)delay/WIDTH << " lines)" << std::endl;

	return 0;
}
