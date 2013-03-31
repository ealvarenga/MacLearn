#ifndef __ARFF_H__
#define __ARFF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "Dataset.h"

	void Arff_Free (DataSet * dataset);
	int Arff_LoadDataset (char * datasetName, DataSet * dataset, DataSetType type);
	int Arff_SaveDataset (char * datasetName, DataSet * dataset);

#ifdef __cplusplus
}
#endif

#endif
