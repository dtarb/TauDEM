/*
 ============================================================================
 Name        : demverifier.c
 Author      : Ahmet Yildirim
 Version     : 1.0
 Description : Checks the elevation differences between two DEM files and reports the result.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <gdal.h>
#include <cpl_conv.h>
#include <stdint.h>
#include <stdlib.h>

/* maximum elevation difference between two cells to be regarded as an "error" */
const float error = 0.000000000001f;

float* get_tif_data(char* tif_file, int* tif_width, int* tif_height) {
	GDALDatasetH hDataset;

	hDataset = GDALOpen(tif_file, GA_ReadOnly);
	if (hDataset == NULL ) {
		fprintf(stderr, "ERROR: Failed to open the file %s\n", tif_file);
		return NULL ;
	}
	GDALRasterBandH hBand;

	hBand = GDALGetRasterBand(hDataset, 1);
	int width1 = GDALGetRasterXSize(hDataset);
	int height1 = GDALGetRasterYSize(hDataset);
	double nodata1 = GDALGetRasterNoDataValue(hBand, NULL );

	float* data = (float *) CPLMalloc(sizeof(float) * width1 * height1);
	if (!data) {
		fprintf(stderr, "ERROR: Failed to allocate data of size %d \n", width1 * height1);
		return NULL;
	}
	GDALRasterIO(hBand, GF_Read, 0, 0, width1, height1, data, width1, height1,
			GDT_Float32, 0, 0);

	*tif_width = width1;
	*tif_height = height1;
	return data;
}

int compare_tiffs(char* tif_file1, char* tif_file2) {
	GDALAllRegister();
	int tif_width1, tif_height1;
	int tif_width2, tif_height2;

	float* data1 = get_tif_data(tif_file1, &tif_width1, &tif_height1);
	if (!data1)
		return 1;
		
	float* data2 = get_tif_data(tif_file2, &tif_width2, &tif_height2);
	if (!data2)
		return 1;

	if ((tif_width1 != tif_width2) || (tif_height1 != tif_height2)) {
		fprintf(stderr, "ERROR: Resolutions is not matching\n");
		return 1;
	}

	int x, y;
	int insdetected = 0;
	float d1, d2;
	for (y = 0; y < tif_height1; y++) {
		for (x = 0; x < tif_width1; x++) {
			d1 = data1[y * tif_width1 + x];
			d2 = data2[y * tif_width1 + x];

			if (fabs(d1 - d2) > error) {
				printf("Inconsistency detected at x:%d, y:%d - ", x, y);
				printf("Data1: %f, Data2: %f\n", d1, d2);
				insdetected = 1;
			}
		}
	}

	free(data1);
	free(data2);

	if (insdetected == 0)
		printf("No inconsistency detected!\n");
	
	return 0;
}

int main(int argc, char* argv[]) {	 
	if (argc != 3) {
		printf("USAGE: program firstdemfilepath seconddemfilepath\n");
		return 1;
	 }
	 
	return compare_tiffs(argv[1], argv[2]);
}
