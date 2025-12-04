#include "axipng.h"

int pngread (const std::string &filename, unsigned char **idata, struct spng_ihdr *ihdr) {
    int ret;

    FILE *png = fopen(filename.c_str(), "rb");
    if (png == NULL) {
        printf("error opening input file %s\n", filename.c_str());
        return 1;
    }

    spng_ctx *ctx = spng_ctx_new(0);
    if (ctx == NULL) {
        printf("error creating spng context\n");
        return 1;
    }

    spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
    size_t limit = 1024 * 1024 * 64;
    spng_set_chunk_limits(ctx, limit, limit);

    // Start decoding
    spng_set_png_file(ctx, png);

    ret = spng_get_ihdr(ctx, ihdr);
    if (ret) {
        printf("error obtaining image header: %s\n", spng_strerror(ret));
        return 1;
    }
    // Force to truecolor + alpha
	ihdr->color_type = SPNG_COLOR_TYPE_TRUECOLOR;

    size_t image_size;
    ret = spng_decoded_image_size(ctx, SPNG_FMT_RGB8, &image_size);
    if (ret) {
        printf("error obtaining image size\n");
        return 1;
    }

    *idata = (unsigned char*) malloc(image_size);
    if (*idata == NULL) {
        printf("error allocating image memory\n");
        return 1;
    }

    ret = spng_decode_image(ctx, *idata, image_size, SPNG_FMT_RGB8, 0);
    if (ret) {
        printf("error decoding image: %s\n", spng_strerror(ret));
        return 1;
    }

    spng_ctx_free(ctx);
    fclose(png);

    return 0;
}

int pngwrite (const std::string &filename, const unsigned char *idata, struct spng_ihdr *ihdr) {
    int ret;

    FILE *png = fopen(filename.c_str(), "wb");
    if (png == NULL) {
        printf("error opening output file %s\n", filename.c_str());
        return 3;
    }

    spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);
    if (ctx == NULL) {
        printf("error creating spng context\n");
        return 3;
    }

    spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, 1);

    spng_set_png_file(ctx, png);

    spng_set_ihdr(ctx, ihdr);

    size_t image_size = ihdr->width * ihdr->height * 3;
    ret = spng_encode_image(ctx, idata, image_size, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
    if (ret) {
        printf("error encoding image: %s\n", spng_strerror(ret));
        return 3;
    }

    spng_ctx_free(ctx);
    fclose(png);

    return 0;
}

int img2axi (const unsigned char *idata, const struct spng_ihdr &ihdr, pixel_stream &stream) {
	pixel_data pixel;

	for (int row = 0; row < ihdr.height; row++) {
		for (int col = 0; col < ihdr.width; col++) {
			pixel.user = (row == 0 && col == 0) ? 1 : 0;
			pixel.last = (col == ihdr.width-1) ? 1 : 0;

			const unsigned char *pdata = &idata[(row * ihdr.width + col) * 3];
			pixel.data = (pdata[0])
					   | (pdata[1] << 8)
					   | (pdata[2] << 16)
					   | (0xFF     << 24);
			stream << pixel;
		}
	}

	return 0;
}
