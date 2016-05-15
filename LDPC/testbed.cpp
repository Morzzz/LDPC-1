#include <iostream>
#include "NBLdpcDecoder.h"
#include "NBLdpcBuilder.h"
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

char *NextArgument()
{
	static int curArg = 1;
	return __argv[curArg++];
}

void Usage()
{
	cout << "Usage:\n"
		<< "B Length Redundancy Extension [R RowVals ColVals] specFileName\n"
		<< "T specFileName NumOfIterations\n"
		<< "D specFileName NumOfDecoderIterations NumOfRemainingVals SNR NumOfErrors NumOfIterations\n";
}

inline float CalculateSigma(float SNR, unsigned Length, unsigned BinDimension) {
	return sqrtf(1.f / (2.f * powf(10.f, (SNR / 10.f)) * ((float)BinDimension / Length)));
}

int main()
{
	if(__argc != 9 && __argc != 4 && __argc != 8)
	{
		Usage();
		return 0;
	}
	char mode = NextArgument()[0];
	unsigned Length, Extension;
	string specFileName;
	if(mode == 'B')
	{
		Length = stoi(NextArgument());
		unsigned Redundancy = stoi(NextArgument());
		Extension = stoi(NextArgument());
		char type = NextArgument()[0];
		if(type == 'R')
		{
			unsigned RowVals = stoi(NextArgument()),
				ColVals = stoi(NextArgument());
			specFileName = NextArgument();
			NBLdpcBuilder::BuildRegular(Length, Redundancy, Extension, RowVals, ColVals, specFileName);
		}
	}
	else if(mode == 'T')
	{
		specFileName = NextArgument();
		unsigned NumOfIterations = stoi(NextArgument());
		NBLdpcCodec codec(specFileName);

		GFSymbol *pData = new GFSymbol[codec.GetDimension()];
		GFSymbol *pEncoded = new GFSymbol[codec.GetLength()];
		mt19937 gen(time(nullptr));
		uniform_int_distribution<int> distr(1, codec.GetMaxFieldElement());
		int errNum = 0;
		for (unsigned i = 0; i < NumOfIterations; ++i)
		{
			transform(pData, pData + codec.GetDimension(), pData, [&](GFSymbol a) { return (GFSymbol)distr(gen); });
			codec.Encode(pData, pEncoded);
			bool result = codec.VerifyCodeword(pEncoded);
			if (!result)
			{
				cout << "Verification fail at iteration #" << i << endl;
				errNum++;
			}			
		}
		if (!errNum)
			cerr << "Verification successful" << endl;
		else
			cerr << "Verification failed" << endl;

		delete[] pData;
		delete[] pEncoded;
	}
	else if (mode == 'D')
	{
		specFileName = NextArgument();
		unsigned NumOfDecoderIterations = stoi(NextArgument());
		unsigned NumOfRemainingVals = stoi(NextArgument());
		float SNR = stof(NextArgument());
		unsigned NumOfErrors = stoi(NextArgument());
		unsigned NumOfIterations = stoi(NextArgument());

		NBLdpcDecoder codec(specFileName, NumOfRemainingVals);
		Length = codec.GetLength();
		Extension = codec.GetFieldOrder();
		unsigned Dimension = codec.GetDimension();
		float Sigma = CalculateSigma(SNR, Length, Dimension * Extension);
		mt19937 engine(time(nullptr));
		normal_distribution<float> noiseGen(0, Sigma);
		uniform_int_distribution<int> dataGen(0, codec.GetMaxFieldElement());
		GFSymbol *pData = new GFSymbol[Dimension];
		GFSymbol *pEncoded = new GFSymbol[Length];
		GFSymbol *pDecoded = new GFSymbol[Length];
		float *pNoisyData = new float[Length];
		unsigned currentErrors = 0;
		unsigned it = 0;
		for (it; it < NumOfIterations && currentErrors < NumOfErrors; ++it)
		{
			for (unsigned i = 0; i < Dimension; i++)
				pData[i] = dataGen(engine);
			codec.Encode(pData, pEncoded);
			for (unsigned i = 0; i < Length; i++)
				pNoisyData[i] = codec.Modulate(pEncoded[i]) + noiseGen(engine);
			codec.Decode(pNoisyData, pDecoded, NumOfDecoderIterations, Sigma * Sigma);

			if (memcmp(pDecoded, pEncoded, sizeof(GFSymbol) * Length) != 0)
				currentErrors++;
			
			if (it > 0 && it % 1000 == 0)
				cout << it << ' ' << (static_cast<float>(currentErrors) / it) <<  endl;
		}
		cerr << it << ' ' << (static_cast<float>(currentErrors) / it) << endl;
		system("pause");
		delete[] pData;
		delete[] pEncoded;
		delete[] pDecoded;
		delete[] pNoisyData;
	}
	return 0;
}