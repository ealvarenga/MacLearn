#ifndef __C50FILE_H__
#define __C50FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "Dataset.h"

	void C50File_Free (DataSet * dataset);
	int C50File_LoadDataset (char * datasetName, DataSet * dataset, DataSetType type);
	int C50File_SaveDataset (char * datasetName, DataSet * dataset);

#ifdef __cplusplus
}
#endif

#endif
