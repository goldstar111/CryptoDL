/*
 * LayerImpl.h
 *
 *  Created on: Feb 20, 2019
 *      Author: robert
 */

#ifndef ARCHITECTURE_LAYERIMPL_H_
#define ARCHITECTURE_LAYERIMPL_H_

#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>
#include <utility>
#include <omp.h>
#include <thread>
#include <memory>
#include <chrono>
#include <type_traits>
#include "ActivationFunction.h"
#include "Tensor.h"
#include "TensorFactory.h"
#include "LoadModelData.h"
#include "../tools/Config.h"


#include "HEBackend/helib/HELIbCipherText.h"



// TODO safeguards for layers without input tensors.
//FIXME: change all TensorFactory* to TensorFactoryP


namespace {
	const bool DEBUG = std::getenv( "DEBUG_LAYER" ) || Config::getConfig()->get<bool>( "general", "debug_layer");
	const unsigned int POOL_SIZE = std::thread::hardware_concurrency() * 2;
}
//const uint POOL_SIZE = 1; // for debugging


template< class Preconvert, class Postconvert>
class WeightConverter {
public:
	virtual TensorP<Postconvert> convert( TensorP<Preconvert> ) = 0;

	virtual ~WeightConverter();
};



template<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>
class Layer {
public:

//	//old
	Layer( std::string name, std::shared_ptr<Activation<ValueType>> act, TensorP<ValueType> mInput,	TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory ) :
			mName( name ), mActivation( act ), mInput( mInput ),mDataTensorFactory( dataTensorFactory ),mWeigthTensorFactory( weigthTensorFactory ) {
	}

	Layer( std::string name, ActivationP<ValueType> act, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory ) :
		mName( name ), mActivation( act ), mDataTensorFactory( dataTensorFactory ),	mWeigthTensorFactory( weigthTensorFactory ) {
	}

	//new
	Layer( std::string name, ActivationP<ValueType> act, TensorP<ValueType> mInput, TensorFactoryP<ValueType> dataTensorFactory, TensorFactoryP<WeightType> weightTensorFactory ) :
			mName( name ), mActivation( act ), mInput( mInput ), mDTF( dataTensorFactory ), mWTF( weightTensorFactory ) {
	}

	Layer( std::string name, ActivationP<ValueType> act, TensorFactoryP<ValueType> dataTensorFactory, TensorFactoryP<WeightType> weightTensorFactory ) :
			mName( name ), mActivation( act ), mDTF( dataTensorFactory ), mWTF( weightTensorFactory ) {
	}

	Layer( std::string name, ActivationP<ValueType> act, TensorP<ValueType> mInput )
			: mName( name ), mActivation( act ), mInput( mInput ), mDataTensorFactory( nullptr ), mWeigthTensorFactory( nullptr ) {
	}

	Layer( std::string name, ActivationP<ValueType> act )
				: mName( name ), mActivation( act ), mDataTensorFactory( nullptr ), mWeigthTensorFactory( nullptr ) {
	}


	virtual Shape outputShape() = 0;

	virtual void feedForward() = 0;

	/**
	 * @brief Wrapper around feedForward that gives some nice print
	 * outs like the layer name and the execution time
	 */
	void run(){
		std::cout << name() << std::flush;
		auto startLayer = std::chrono::system_clock::now();
		feedForward();
		auto endLayer = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_layer = endLayer - startLayer;
		std::cout << " " << elapsed_seconds_layer.count() << "s" << std::endl;
	}

	virtual void loadWeights( std::string path, std::string fileName ) = 0;

	virtual void loadWeights( std::string path ) = 0;

	virtual void randomInit() = 0;

	/**
	 * @brief This function is supposed to be used if a layer needs to build a special output layer.
	 * It will get called when layers gets added to the model. Build yuour own output layer in this function and
	 * return false.
	 *
	 * See Flatten for an example
	 */
	virtual bool buildsOwnOutputTensor() {
		return false;
	}

	/**
	 * @brief Call if additional setup is needed.Does nothing by default
	 */
	virtual void setup(){
	}


	virtual void description() {
	}

	//getters
	std::string name() {
		return mName;
	}


	TensorP<ValueType> input() {
		return this->mInput;
	}

	TensorP<ValueType> output() {
		return this->mOutput;
	}


	//setters
	void input( TensorP<ValueType> input ) {
		this->mInput = input;
	}

	void output( TensorP<ValueType> output ) {
		this->mOutput = output;
	}

	void activation( ActivationP<ValueType> act ) { //used to set the activation function
		mActivation = act;
	}

	void weights( TensorP<WeightType> weights ) {
		this->mWeights = weights;
	}

	void biases( TensorP<WeightType> biases ) {
		this->mBiases = biases;
	}

	TensorP<WeightType> weights(  ) {
		return this->mWeights;
	}

	TensorP<WeightType> biases( ) {
		return this->mBiases;
	}

	/**
	 * @brief Returns all the weights and bias tensors of the layer. Order is up to the individual layer.
	 */
	virtual std::vector<TensorP<WeightType>> allWeights() = 0;

	/**
	 * @brief Set the convertedWeights vector. The order needs to be the same as returned by allWeights()
	 */
	void convertedWeights( std::vector<TensorP<ConvertedWeights>>& convertedWeights ){
		mConvertedWeights = convertedWeights;
	}

	virtual ~Layer() {
	}

	void clear(){
		mInput->clear();
		mConvertedWeights.clear();
	}


protected:
	std::string mName; //stores the layer's name
	ActivationP<ValueType> mActivation; //will point to derived class

	TensorP<ValueType> mInput = NULL, mOutput;
	TensorP<WeightType> mWeights, mBiases;
	TensorFactory<ValueType>* mDataTensorFactory; //TODO: remove these
	TensorFactory<WeightType>* mWeigthTensorFactory;

	TensorFactoryP<ValueType> mDTF; //will point to appropriate tensor factory
	TensorFactoryP<WeightType> mWTF;

	std::vector<TensorP<ConvertedWeights>> mConvertedWeights;


private:
	template<class V, class W, class DT, class WT, class CT>
	friend class Model;

};



template<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>
using LayerP = std::shared_ptr<Layer<ValueType, WeightType, DataTensorType, WeightTensorType>>;


/***
 *
 * The padding semantics used are the same as being used by tensorflow. This
 * means if we use SAME_PADDING with a non unit stride and need to pad an
 * uneven amount of pixels the bottom and right sides always get the one additional
 * padded pixel.
 *
 * For more information see:
 * TODO: The link may not exist
 * https://www.tensorflow.org/api_guides/python/nn#Convolution
 * https://www.tensorflow.org/api_docs/python/tf/nn/conv2d
 */
enum PADDING_MODE {
	SAME, VALID
};

template<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>
class Convolution2D: public Layer<ValueType, WeightType, DataTensorType, WeightTensorType, ConvertedWeights> {
public:
	Convolution2D( std::string name, ActivationP<ValueType> act, uint noFilters, uint filterSize, uint stride, PADDING_MODE pad, TensorP<ValueType> input, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input, dataTensorFactory, weigthTensorFactory ), mNoFilters( noFilters ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Convolution2D( std::string name, ActivationP<ValueType> act, uint noFilters, uint filterSize, uint stride, PADDING_MODE pad, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, dataTensorFactory, weigthTensorFactory ), mNoFilters( noFilters ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
	}

	Convolution2D( std::string name, ActivationP<ValueType> act, uint noFilters, uint filterSize, uint stride, PADDING_MODE pad, TensorP<ValueType> input )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input ), mNoFilters( noFilters ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Convolution2D( std::string name, ActivationP<ValueType> act, uint noFilters, uint filterSize, uint stride, PADDING_MODE pad )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act ), mNoFilters( noFilters ),	mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
	}

	//new ones
	Convolution2D( std::string name, ActivationP<ValueType> act, uint noFilters, uint filterSize, uint stride, PADDING_MODE pad, TensorP<ValueType> input, TensorFactoryP<ValueType> dataTensorFactory,	TensorFactoryP<WeightType> weightTensorFactory ) :
					Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input, dataTensorFactory, weightTensorFactory ),	mNoFilters( noFilters ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
		this->mOutput = this->mDTF.get()->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Convolution2D( std::string name, ActivationP<ValueType> act, uint noFilters,
			uint filterSize, uint stride, PADDING_MODE pad,
			TensorFactoryP<ValueType> dataTensorFactory,
			TensorFactoryP<WeightType> weightTensorFactory ) :
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name, act, dataTensorFactory,
							weightTensorFactory ), mNoFilters( noFilters ),
					mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
	}


	// computes the output shape for the layer
	Shape outputShape() override {
		uint outputSizeX, outputSizeY;
		if ( mPad == SAME ) {
			// x direction
			assert( mStride != 0 && "Do not want to mod 0" );
			assert( this->mInput->shape.size >= 4 );

			if ( this->mInput->shape [ 3 ] % mStride == 0 ) {
				assert( mFilterSize - mStride >= 0 );
			} else {
				assert( ( mFilterSize - ( this->mInput->shape[ 3 ] % mStride ) ) >= 0 );
			}
			// y direction
			if ( this->mInput->shape [ 2 ] % mStride == 0 ) {
				assert( mFilterSize - mStride >= 0 );
			}
			else {
				assert( ( mFilterSize - ( this->mInput->shape[ 2 ] % mStride ) ) >= 0 );
			}
			outputSizeX = std::ceil( this->mInput->shape[ 3 ] / ( mStride * 1.0 ) );
			outputSizeY = std::ceil( this->mInput->shape[ 2 ] / ( mStride * 1.0 ) );
		} else {
			outputSizeX = std::ceil( ( this->mInput->shape[ 3 ] - mFilterSize + 1 ) / ( mStride * 1.0 ) );
			outputSizeY = std::ceil( ( this->mInput->shape[ 2 ] - mFilterSize + 1 ) / ( mStride * 1.0 ) );
		}
		return Shape { this->mInput->shape[ 0 ], mNoFilters, outputSizeX, outputSizeY };
	}

	void feedForward() override {
		std::thread tt [ POOL_SIZE ];
		uint j = 0;
		while ( j < mNoFilters ) {
			int noThreads = 0;
			for ( unsigned int i = 0; i < POOL_SIZE; i++ ) {
				tt [ i ] = std::thread( [=] {this->filterOperation( j );} );
				j++;
				noThreads = i + 1;
				if ( j == mNoFilters )
					break;
			}
			for ( int i = 0; i < noThreads; i++ )
				tt[ i ].join();
		}
		this->mOutput->performChecks();
	}

	void loadWeights( std::string path, std::string fileName ) override {
		// load the weights from file
		std::pair<std::vector<std::vector<WeightType>>, std::vector<WeightType>> p;
		p = loadFilterWeights<WeightType>( path + fileName, mNoFilters, this->mInput->shape [ 1 ], mFilterSize );
		//is a mxn vector
		std::vector<std::vector<WeightType>> weightVector = p.first;
		//is a 1 x n vector
		std::vector<WeightType> biasVector = p.second;
		//trying to make it a filterx1x(sizexsize)
		this->mWeights = this->mWeigthTensorFactory->create( Shape { mNoFilters * this->mInput->shape [ 1 ], mFilterSize * mFilterSize } ); // this will be reshaped later
		//trying to make a 1xn (ok)
		this->mBiases = this->mWeigthTensorFactory->create(	Shape { mNoFilters });

		//copy the actual data
		this->mWeights->init( weightVector );
		this->mBiases->init( biasVector );

		// need to reshape the weightVector because the data is loaded flat
		this->mWeights->reshape( Shape { mNoFilters, this->mInput->shape[ 1 ], mFilterSize, mFilterSize } );
	}

	void loadWeights( std::string path ) override {
		loadWeights( path, this->mName );
	}

	virtual void randomInit() override {
		this->mWeights->initRandom();
		this->mBiases->initRandom();
	}

	void description() override {
		std::cout << this->mName << "\t" << this->mOutput->shape << std::endl;
	}

	/**
	 * @brief Returns weights, biases
	 */
	virtual std::vector<TensorP<WeightType>> allWeights() override {
		return std::vector<TensorP<WeightType>>{ this->mWeights, this->mBiases };
	}


private:
	uint mNoFilters, mFilterSize, mStride;
	PADDING_MODE mPad;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
	void filterOperation( uint sequence ) {

		bool useConvertedWeights = this->mConvertedWeights.size() < 0;
		TensorP<ConvertedWeights> cWeights;
		TensorP<ConvertedWeights> cBiases;
		if( useConvertedWeights ){
			cWeights = this->mConvertedWeights[ 0 ];
			cBiases = this->mConvertedWeights[ 1 ];
		}

		//run operation using same padding mode
		if ( mPad == PADDING_MODE::SAME ) {
			//determine padding amounts
			int pady, padTop, padLeft;
			if ( this->mInput->shape [ 3 ] % mStride == 0 )
				pady = std::max<int>( mFilterSize - mStride, 0 );
			else
				pady = std::max<int>( mFilterSize - ( this->mInput->shape [ 2 ] % mStride ), 0 );
			padLeft = padTop = pady / 2; 		// because symmetry

			//iterate over each batch
			for ( unsigned int batchIdx = 0; batchIdx < this->mInput->shape [ 0 ]; batchIdx++ ) { // we need to makes some changes here for SIMD
				//iterate all "subfilters"
				for ( unsigned int depthIdx = 0; depthIdx < this->mInput->shape [ 1 ]; ++depthIdx ) {
					int outIdx = 0;
					uint outX = 0, outY = 0;
					// iterating over the image rows
					for ( int y = 0; y < (signed) this->mInput->shape [ 2 ]; y += mStride ) {
						// iterating over the image columns
						for ( int x = 0; x < (signed) this->mInput->shape [ 3 ]; x += mStride ) {
							//iter over filter cols
							for ( unsigned int filtery = 0;	filtery < mFilterSize; filtery++ ) {
								int iy = y + filtery - padTop;
								//iter over filter rows
								for ( unsigned int filterx = 0; filterx < mFilterSize; filterx++ ) {
									int ix = x + filterx - padLeft;
									//we ignore any padded image area since n * 0 = 0 so it adds nothing to sum
									if ( ix >= 0
											&& ix < (signed) this->mInput->shape [ 3 ]
											&& iy >= 0
											&& iy < (signed) this->mInput->shape [ 2 ] ) {
										ValueType value = this->mInput->empty();
										value += ( *this->mInput ) [ { batchIdx, depthIdx, iy, ix } ];
										if( useConvertedWeights )
											value *= ( *cWeights ) [ { sequence, depthIdx, filtery, filterx } ];
										else
											value *= ( *this->mWeights ) [ { sequence, depthIdx, filtery, filterx } ];
										( *this->mOutput ) [ { batchIdx, sequence, outY, outX } ] += value;
										//TODO: do we have to use empty()? Maybe for HE but even then, value should be the same?
									}
								}
							}

							outIdx++;
							// calculate the indices for the output pixels
							outY = outIdx / this->mOutput->shape [ 2 ];
							outX = outIdx % this->mOutput->shape [ 3 ];
						}
					}
				}
				//FIXME this will be way cooler with slicing
				//run activation function over the output
				for ( int y = 0; y < (signed) this->mOutput->shape [ 2 ]; ++y ) {
					for ( int x = 0; x < (signed) this->mOutput->shape [ 3 ]; ++x ) {
						if( useConvertedWeights )
							( *this->mOutput ) [ { batchIdx, sequence, y, x } ] += ( *cBiases ) [ { sequence } ];
						else
							( *this->mOutput ) [ { batchIdx, sequence, y, x } ] += ( *this->mBiases ) [ { sequence } ];
						this->mActivation->activate( ( *this->mOutput ) [ { batchIdx, sequence, y, x } ] );
					}
				}
			}
		}
		else {				// valid padding is used
			int filterFrom, filterTo; /// for even and uneven sized filters
			if ( mFilterSize % 2 == 0 ) {
				filterFrom = - ((signed) mFilterSize / 2 );
				filterTo = ( (signed) mFilterSize / 2 ) - 1;

			} else {
				filterFrom = - ( (signed)mFilterSize / 2 );
				filterTo = (signed) mFilterSize / 2;

			}
			for ( unsigned int batchIdx = 0; batchIdx < this->mInput->shape[ 0 ]; batchIdx++ ) { // we need to makes some changes here for SIMD
				for ( unsigned int depthIdx = 0; depthIdx < this->mInput->shape[ 1 ]; ++depthIdx ) { // all "subfilters"
					int outIdx = 0;
					uint outX = 0, outY = 0;
					for ( int y = 0; y < (signed) this->mInput->shape[ 2 ]; y += mStride ) { // iterating over the image rows
						for ( int x = 0; x < (signed) this->mInput->shape[ 3 ]; x += mStride ) { // iterating over the image columns
							bool validFilter = true; // to break out of inner filter loop
							ValueType weightedSum = this->mInput->empty();
							for ( int filtery = filterFrom; filtery <= filterTo; filtery++ ) { // iterating over the filter rows
								int iy = y + filtery;
								// check if we are in a valid filter area
								// if at least on filter is not abort and move filter
								if ( iy < 0 || iy >= (signed) this->mInput->shape[ 2 ] || !validFilter ) {
									validFilter = false;
									break;
								}
								for ( int filterx = filterFrom; filterx <= filterTo; filterx++ ) { // iterating over the filter columns
									int ix = x + filterx;
									// check if we are in a valid filter area
									if ( ix < 0 || ix >= (signed) this->mInput->shape[ 3 ] ) {
										validFilter = false;
										break;
									}
									ValueType temp = this->mInput->empty();
									if( DEBUG ){
										if( outY == 0 && outX == 0 && sequence == 0 )
											std::cout << "initial noise: " << reinterpret_cast<HELibCipherText*>(&temp)->mCtxt->getNoiseBound() << std::endl;
									}
									temp += ( *this->mInput ) [ { batchIdx, depthIdx, iy, ix } ];
									if( DEBUG ){
										if( outY == 0 && outX == 0 && sequence == 0 )
											std::cout << "adding plain weight: " << reinterpret_cast<HELibCipherText*>(&temp)->mCtxt->getNoiseBound() << std::endl;
									}
									if( useConvertedWeights )
										temp *= ( *cWeights ) [ { sequence, depthIdx, ( filtery + mFilterSize / 2 ), ( filterx + mFilterSize / 2 ) } ];
									else
										temp *= ( *this->mWeights ) [ { sequence, depthIdx, ( filtery + mFilterSize / 2 ), ( filterx + mFilterSize / 2 ) } ];
									if( DEBUG ){
										if( outY == 0 && outX == 0 && sequence == 0 )
											std::cout << "mult by input : " << reinterpret_cast<HELibCipherText*>(&temp)->mCtxt->getNoiseBound() << std::endl;
									}
									weightedSum += temp;
								}
							}
							if ( validFilter ) {
								( *this->mOutput ) [ { batchIdx, sequence, outY, outX } ] += weightedSum;
								++outIdx;
								// calculate the indices for the output pixels
								outY = outIdx / this->mOutput->shape[ 2 ];
								outX = outIdx % this->mOutput->shape[ 3 ];

							}
						}
					}
				}
				//FIXME this will be way cooler with slicing//TODO: this prob needs to be inside depthIdx forloop
				for ( int y = 0; y < (signed) this->mOutput->shape[ 2 ]; ++y ) {
					for ( int x = 0; x < (signed) this->mOutput->shape[ 3 ]; ++x ) {
						if( useConvertedWeights )
							( *this->mOutput )[ { batchIdx, sequence, y, x } ] += ( *cBiases )[ { sequence } ];
						else
							( *this->mOutput )[ { batchIdx, sequence, y, x } ] += ( *this->mBiases )[ { sequence } ];
						this->mActivation->activate( ( *this->mOutput )[ { batchIdx, sequence, y, x } ] );
						if( DEBUG ){
							if( sequence == 0 && x == 0 && y ==0 )
								std::cout << "activation function: " << (reinterpret_cast<HELibCipherText*>(&( *this->mOutput )[ { batchIdx, sequence, 0, 0 } ]))->mCtxt->getNoiseBound() << std::endl;
						}
					}
				}
			}
		}
	}


#pragma GCC diagnostic pop

};

template<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>
class Dense: public Layer<ValueType, WeightType, DataTensorType, WeightTensorType, ConvertedWeights> {
public:

	//old
	Dense( std::string name, ActivationP<ValueType> act, uint noNeurons, TensorP<ValueType> input,
			TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input, dataTensorFactory, weigthTensorFactory ), mNoNeurons( noNeurons ) {
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Dense( std::string name, ActivationP<ValueType> act, uint noNeurons, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, dataTensorFactory, weigthTensorFactory ), mNoNeurons( noNeurons ) {
	}

	Dense( std::string name, ActivationP<ValueType> act, uint noNeurons, TensorP<ValueType> input)
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input ), mNoNeurons( noNeurons ) {
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Dense( std::string name, ActivationP<ValueType> act, uint noNeurons )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act ), mNoNeurons( noNeurons ) {
	}
	//new
	Dense( std::string name, ActivationP<ValueType> act, uint noNeurons,
			TensorP<ValueType> input,
			TensorFactoryP<ValueType> dataTensorFactory,
			TensorFactoryP<WeightType> weightTensorFactory ) :
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name, act, input,
							dataTensorFactory, weightTensorFactory ),
					mNoNeurons( noNeurons ) {
//		this->mOutput = this->mDTF.get()->create( outputShape() );
		this->mOutput = this->mDataTensorFactory->create( outputShape() );

	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Dense( std::string name, ActivationP<ValueType> act, uint noNeurons,
			TensorFactoryP<ValueType> dataTensorFactory,
			TensorFactoryP<WeightType> weightTensorFactory ) :
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name, act, dataTensorFactory,
							weightTensorFactory ), mNoNeurons( noNeurons ) {
	}


	// computes the outputshape for the layer
	Shape outputShape() override {
		return Shape { this->mInput->shape[ 0 ], this->mNoNeurons };
	}

	void feedForward() override {
		// sanity checks
		std::thread tt  [ POOL_SIZE ];
		uint j = 0;
		while( j < mNoNeurons ){
			int noThreads = 0;
			for ( uint i = 0; i < POOL_SIZE; i++ ) {
				tt [ i ] = std::thread( [=] {this->neuron( j );} );
				j++;
				noThreads = i + 1;
				if ( j == mNoNeurons )
					break;
			}
			for ( int i = 0; i < noThreads; i++ )
				tt[ i ].join();
		}
		//this->mOutput->performChecks();
	}

	void loadWeights( std::string path, std::string fileName ) {
		//load weights from file
		std::vector<std::vector<WeightType>> weightsVector = loadingFCWeights<WeightType>( path + this->mName,
				mNoNeurons );
		//load biases from file
		std::vector<WeightType> biasesVector = loadFilterWeights<WeightType>( path + this->mName + "_bias.txt" );

		//create tensors and copy values into it
		//TODO
		this->mWeights = this->mWeigthTensorFactory->create( Shape { mNoNeurons, this->mInput->shape [ 1 ] } );
		this->mWeights->init( weightsVector );
		this->mBiases = this->mWeigthTensorFactory->create( Shape { mNoNeurons } );
		this->mBiases->init( biasesVector );
	}



	void loadWeights( std::string path ) {
		loadWeights( path, this->mName );
	}

	virtual void randomInit() override {
			this->mWeights->initRandom();
			this->mBiases->initRandom();
	}

	void description() {
		std::cout << this->mName << "\t\t" << this->mOutput->shape << std::endl;
	}

	/**
	 * @brief Returns weights, biases
	 */
	virtual std::vector<TensorP<WeightType>> allWeights() override {
		return std::vector<TensorP<WeightType>>{ this->mWeights, this->mBiases };
	}

private:
	uint mNoNeurons;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"


	void neuron( uint sequence ) { // TODO redo with slicing

		bool useConvertedWeights = this->mConvertedWeights.size() < 0;
		TensorP<ConvertedWeights> cWeights;
		TensorP<ConvertedWeights> cBiases;
		if( useConvertedWeights ){
			cWeights = this->mConvertedWeights[ 0 ];
			cBiases = this->mConvertedWeights[ 1 ];
		}

		for ( uint batchIdx = 0; batchIdx < this->mInput->shape [ 0 ]; ++batchIdx ) {
			//TODO this could be done more efficently if we did not create need to create empties
			ValueType temp = this->mInput->empty();
			for ( uint i = 0; i < this->mWeights->shape [ 1 ]; ++i ) {
				temp = this->mInput->empty();
				temp += ( *this->mInput ) [ { batchIdx, i } ];
				if( useConvertedWeights )
					temp *= ( *cWeights ) [ { sequence, i } ];
				else
					temp *= ( *this->mWeights ) [ { sequence, i } ];
				( *this->mOutput ) [ { batchIdx, sequence } ] += temp;
			}
			if( useConvertedWeights )
				( *this->mOutput ) [ { batchIdx, sequence } ] += ( *cBiases )[ { sequence } ];
			else
				( *this->mOutput ) [ { batchIdx, sequence } ] += ( *this->mBiases )[ { sequence } ];
			this->mActivation->activate( ( *this->mOutput )[ { batchIdx, sequence } ] );
		}
	}

#pragma GCC diagnostic pop

};


// Flatten does not provide a copy the data. It works inplace on the input tensor
template<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>
class Flatten: public Layer<ValueType, WeightType, DataTensorType, WeightTensorType> {
public:

	Flatten( std::string name, TensorP<ValueType> input, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory, bool channelsFirst=false )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer(), input, dataTensorFactory, weigthTensorFactory ), mChannelsFirst( channelsFirst )
	{
		buildOutputTensor();
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Flatten( std::string name, TensorFactory<ValueType>* dataTensorFactory,
			TensorFactory<WeightType>* weightTensorFactory, bool channelsFirst=false ) :
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name,
							LinearActivation<ValueType>::getSharedPointer(),
							dataTensorFactory, weightTensorFactory ), mChannelsFirst( channelsFirst ) {
	}

	//new
	Flatten( std::string name, TensorP<ValueType> input,
			TensorFactoryP<ValueType> dataTensorFactory,
			TensorFactoryP<WeightType> weightTensorFactory, bool channelsFirst=false ) :
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name,
							LinearActivation<ValueType>::getSharedPointer(),
							input, dataTensorFactory, weightTensorFactory ), mChannelsFirst( channelsFirst ) {
		buildOutputTensor();
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	// We pass in tensor factory shared pointers to allow for the factory to exist across different scopes
	//TODO: do this for all layers (replace * wtf with smart* wtf, etc)
	Flatten( std::string name, TensorFactory<ValueType>* dataTensorFactory,
			TensorFactory<WeightType>* weightTensorFactory,
			TensorFactoryP<ValueType> dtf, TensorFactoryP<WeightType> wtf, bool channelsFirst=false )
			:
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name,
							LinearActivation<ValueType>::getSharedPointer(),
							dataTensorFactory, weightTensorFactory ), mChannelsFirst( channelsFirst )
	{
		this->mDTF = dtf;
		this->mWTF = wtf;
	}

	Flatten( std::string name, TensorP<ValueType> input, bool channelsFirst=false)
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer(), input ), mChannelsFirst( channelsFirst )
	{
		buildOutputTensor();
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	Flatten( std::string name, bool channelsFirst=false)
			:
	Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer() ), mChannelsFirst( channelsFirst )
	{
	}



	// computes the output shape for the layer
	Shape outputShape() override {
		uint dim = 1;
		for ( uint i = 1; i < this->mInput->shape.size; ++i )
			dim *= this->mInput->shape[ i ];
		return Shape( { this->mInput->shape [ 0 ], dim } );
	}

	void feedForward() override {
		// channel last is handeled in place
		// for channel first ordering see: https://github.com/tensorflow/tensorflow/blob/r1.13/tensorflow/python/keras/layers/core.py Flatten
		if( mChannelsFirst ){
			auto s = this->mInput->shape;
			this->mOutput->reshape( Shape({ s[0], s[2], s[3], s[1] }) ); //
			// working with input ordering
			for( uint b = 0; b < s[0]; ++b ){
				for( uint c = 0; c < s[1]; ++c ){
					for( uint y = 0; y < s[2]; ++y ){
						for( uint x = 0; x < s[3]; ++x ){
							(*this->mOutput)[ {b, y, x, c} ] = (*this->mInput)[ {b, c, y, x} ] ;
						}
					}
				}
			}
			this->mOutput->reshape( this->outputShape() ); //
		}
		this->mOutput->performChecks();

	}

	bool buildsOwnOutputTensor() override {
		buildOutputTensor();
		return true;
	}

	void loadWeights( std::string path, std::string fileName ) {
		// nothing to do
	}

	void loadWeights( std::string path ) {
		// nothing to do
	}

	virtual void randomInit() override {
	}

	void description() override {
		std::cout << this->mName << "\t" << this->mOutput->shape << std::endl;
	}

	/**
	 * @brief Returns empty vector
	 */
	virtual std::vector<TensorP<WeightType>> allWeights() override {
		return std::vector<TensorP<WeightType>>();
	}

private:
	bool mChannelsFirst = false;

	void buildOutputTensor() {
		if( ! mChannelsFirst )
			this->mOutput = this->mDataTensorFactory->createView( this->outputShape(), this->mInput );
		else{
			if( this->mInput->shape.size != 4 )
				throw std::logic_error( "channel first flatten currently only works on 4D tensors" );
			this->mOutput = this->mDataTensorFactory->create( this->outputShape() );
		}

	}

};


template<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>
class RNN: public Layer<ValueType, WeightType, DataTensorType, WeightTensorType> {
public:

	RNN( std::string name, ActivationP<ValueType> act, uint units, bool returnSequences, TensorP<ValueType> input, TensorFactory<ValueType>* dataTensorFactory,
			TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input, dataTensorFactory,
					weigthTensorFactory ), mUnits( units ), mReturnSquences( returnSequences )
	{
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	RNN( std::string name, ActivationP<ValueType> act, uint units, bool returnSequences, TensorFactory<ValueType>* dataTensorFactory,
			TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, dataTensorFactory, weigthTensorFactory ),
					mUnits( units ), mReturnSquences( returnSequences )
	{
	}

	RNN( std::string name, ActivationP<ValueType> act, uint units, bool returnSequences, TensorP<ValueType> input)
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input), mUnits( units ), mReturnSquences( returnSequences )
	{
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	RNN( std::string name, ActivationP<ValueType> act, uint units, bool returnSequences )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act),
					mUnits( units ), mReturnSquences( returnSequences )
	{
	}

	// computes the outputshape for the layer
	Shape outputShape() override {
		if ( mReturnSquences )
			return Shape( { this->mInput->shape[ 0 ], this->mInput->shape[ 1 ], mUnits } );
		return Shape( { this->mInput->shape[ 0 ], mUnits } );
	}

	/** @brief Setup the innerstates tensor
	 *
	 */
	void setup(){
		// create Tensor to hold intermediate results
		// if memory becomes an issue we can sacrifice here by
		// creating a tensor that only holds the data for one timestep
		if( mReturnSquences )
			innerStates = this->mOutput; // we trust that the  output tensor has been init..ed
		else {
			innerStates = this->mDataTensorFactory->create( { this->mInput->shape[ 0 ], this->mUnits } ); // [ batch,  units ]
			lastInnerStates = this->mDataTensorFactory->create( { this->mInput->shape[ 0 ], this->mUnits } ); // [ batch,  units ]
		}
	}

	void feedForward() override {

		auto threadEnv = std::getenv( "RNN_SINGLE_THREAD" );
		bool singel_threaded = false;
		if( threadEnv  ){
			std::cout << "running single threaded rnn" << std::endl;
			singel_threaded = true;
		} else  {
			if ( DEBUG ) std::cout << "running multi threaded rnn" << std::endl;
			singel_threaded = false;
		}

		TensorP<ValueType> swap; // used for swapping tensors
		

		// maybe looping as welltemplate<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>

		// init/clear
		innerStates->init();
		lastInnerStates->init();


		if( singel_threaded ){
			for ( uint timeIdx = 0; timeIdx < this->mInput->shape[ 1 ]; ++timeIdx ) { // iterate over the timesteps
				auto start = std::chrono::system_clock::now();
				if( DEBUG ) std::cout  << timeIdx << "/" << this->mInput->shape[ 1 ] << std::flush;
				// calculate the hidden units first
				// calclutate part of the state based on the input at the time step
				for ( uint unitIdx = 0; unitIdx < mUnits; ++unitIdx )  // iterate over the hidden units
					hiddenUnit( unitIdx, timeIdx );
				// add the recurrent state
				for ( uint unitIdx = 0; unitIdx < mUnits; ++unitIdx )  // iterate over the hidden uints
					recurrentUnit(unitIdx, timeIdx);
				// add bias and apply activation
				for ( uint unitIdx = 0; unitIdx < mUnits; ++unitIdx )  // iterate over the hidden uints
					activation( unitIdx, timeIdx );
				auto end = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = end - start;
				if( DEBUG ) std::cout << " Time: " << elapsed_seconds.count() << "s" << std::endl;
				this->innerStates->performChecks();
				if( !mReturnSquences ){
					swap = lastInnerStates;
					lastInnerStates = innerStates;
					innerStates = swap;
				}
			}
		}
		else {

			// create index queues used in the activation. could be used in other places as well,
			// TODO and FIXME should probably be used every where
			// i could also be smarter about assembling this queue.
			std::vector<std::vector<uint>> idq( POOL_SIZE, std::vector<uint>() );
			for( uint i = 0; i < mUnits; ++i )
				idq[ i % POOL_SIZE ].push_back( i ); // balance out the indexes over the queues


			std::thread tt  [ POOL_SIZE ];
			for ( uint timeIdx = 0; timeIdx < this->mInput->shape[ 1 ]; ++timeIdx ) { // iterate over the timesteps
				auto start = std::chrono::system_clock::now();
				if( DEBUG ) std::cout << timeIdx << "/" << this->mInput->shape[ 1 ] << std::flush;
				// calclutate part of the state based on the input at the time step
				uint unitIdx = 0;
				while( unitIdx < mUnits  ){ // iterate over the hidden units
					int noThreads = 0;
					for ( uint i = 0; i < POOL_SIZE; i++ ) {
						tt [ i ] = std::thread( [this,unitIdx,timeIdx] {this->hiddenUnit( unitIdx, timeIdx );} );
						++unitIdx;
						noThreads = i + 1;
						if ( unitIdx == mUnits )
							break;
					}
					for ( int i = 0; i < noThreads; i++ )
						tt[ i ].join();
				}

				// add reccurrent state
				unitIdx = 0;
				while( unitIdx < mUnits  ){ // iterate over the hidden units
					int noThreads = 0;
					for ( uint i = 0; i < POOL_SIZE; i++ ) {
						tt [ i ] = std::thread( [this,unitIdx,timeIdx] {this->recurrentUnit( unitIdx, timeIdx );} );
						++unitIdx;
						noThreads = i + 1;
						if ( unitIdx == mUnits )
							break;
					}
					for ( int i = 0; i < noThreads; i++ )
						tt[ i ].join();
				}


				// add bias and apply activation
				uint noThreads = 0;
				for ( auto idxs : idq ) // iterate over index queues
					tt [ noThreads++ ] = std::thread( [this, idxs, timeIdx] {this->activations( idxs, timeIdx );} );
				for ( uint i = 0; i < noThreads; i++ )
					tt[ i ].join();


				if ( timeIdx % 27 == 0 )
					this->innerStates->performChecks();
				auto end = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = end - start;
				if( DEBUG ) std::cout << " Time: " << elapsed_seconds.count() << "s" << std::endl;
				if( !mReturnSquences ){
					swap = lastInnerStates;
					lastInnerStates = innerStates;
					innerStates = swap;
				}
			}
		}
		
		// set output to the result of the last timestep
		if( !mReturnSquences )
			this->mOutput->feed( *lastInnerStates );
		
	}


	void loadWeights( std::string path, std::string fileName ) {
		loadWeights( path, this->mName );
	}

	void loadWeights( std::string path ) {
		//load weigths from file
		std::vector<std::vector<WeightType>> weightsVector = loadingFCWeights<WeightType>( path + this->mName,
				mUnits );
		//load recurrent weigths from file
		std::vector<std::vector<WeightType>> reccurnteWeightsVector = loadingFCWeights<WeightType>( path + this->mName + "_recurrent",	mUnits );
		//load biases from file
		std::vector<WeightType> biasesVector = loadFilterWeights<WeightType>( path + this->mName + "_bias.txt" );

		//create tensors and copy values into it

		this->mWeights = this->mWeigthTensorFactory->create( Shape { mUnits, this->mInput->shape[ 2 ] } );
		this->mWeights->init( weightsVector );
		this->mRecurrentWeights = this->mWeigthTensorFactory->create( Shape { mUnits, mUnits } ); // @suppress("Symbol is not resolved")
		this->mRecurrentWeights->init( reccurnteWeightsVector );
		this->mBiases = this->mWeigthTensorFactory->create( Shape { mUnits } );
		this->mBiases->init( biasesVector );
	}

	virtual void randomInit() override {
			this->mWeights->initRandom();
			this->mBiases->initRandom();
			this->mRecurrentWeights->initRandom( );
	}

	void description() override {
		std::cout << this->mName << std::endl;
		//TODO: other info such as shape, size, etc.
	}

	uint units() const {
		return mUnits;
	}

	TensorP<WeightType> recurrentWeights() const {
		return mRecurrentWeights;
	}

	void recurrentWeights( const TensorP<WeightType>& recurrentWeights ) {
		mRecurrentWeights = recurrentWeights;
	}

	/**
	 * @brief Returns weights, biases, recurrentWeights
	 */
	virtual std::vector<TensorP<WeightType>> allWeights() override {
		return std::vector<TensorP<WeightType>>{ this->mWeights, this->mBiases, this->mRecurrentWeights };
	}


private:
	uint mUnits;
	bool mReturnSquences;
	TensorP<WeightType> mRecurrentWeights;
	TensorP<ValueType> innerStates;
	TensorP<ValueType> lastInnerStates; // we only need this one if we don't return sequences

	/** @brief the hidden untis for the netwrok.
	 * coputes on the hidden units taking only the input into account.
	 * Does not use activation or the recurrent state.
	 *
	 * unitIdx: index of the hidden unit
	 * timeIdx: index in the sequence
	 */
	void hiddenUnit( uint unitIdx, uint timeIdx ){
		/**
		 * Calculation of the hidden unit j ( U[j] in matrix notation) at the time step t. Given inputs I
		 * and weights W is:
		 *
		 * U[j] = sum( W[ i, j ] * I[ i ] ) ; i[ 0, number of inputs ]
		 *
		 */

		bool useConvertedWeights = this->mConvertedWeights.size() < 0;
		TensorP<ConvertedWeights> cWeights;
		if( useConvertedWeights )
			cWeights = this->mConvertedWeights[ 0 ];

		// reset the innter states if we don\t return sequences
		if( ! mReturnSquences )
			for ( uint batchIdx = 0; batchIdx < this->mInput->shape[ 0 ]; ++batchIdx ) { // iterate over the batch
				//std::cout << batchIdx << " " << unitIdx << std::endl;
				( *innerStates )[ { batchIdx, unitIdx } ] = innerStates->empty();
			}

		for ( uint batchIdx = 0; batchIdx < this->mInput->shape[ 0 ]; ++batchIdx ) { // iterate over the batch
			for ( uint inIdx = 0; inIdx < this->mInput->shape[ 2 ]; ++inIdx ) { // iterate over the data dimension
				WeightType w = ( *this->mWeights )[ { unitIdx, inIdx } ];
				// if w is 0 we can just skip the step because it will add nothing
				if ( w == 0 )
					continue;
				ValueType temp = this->mInput->empty(); //TODO this could be done more efficently if we did not create need to create empties
				temp += ( *this->mInput )[ { batchIdx, timeIdx, inIdx } ];
				if( useConvertedWeights )
					temp *= ( *cWeights )[ { unitIdx, inIdx } ];
				else
					temp *= w;

				// if we are in the last time step and are not returning the sequences
				// write to the layer output instead
				if ( mReturnSquences ) {
					( *innerStates )[ { batchIdx, timeIdx, unitIdx } ] += temp;
				}
				else
					( *innerStates )[ { batchIdx, unitIdx } ] += temp;
			}
		}
	}

	void recurrentUnit( uint unitIdx, uint timeIdx ){
		/**
		 * The formula for the hidden recurrent state is as follows
		 * at the timestep t the for hidden unit j, with the previous as IS
		 * and the recurrent weights as RW
		 * (the batch dimension is ommmited for simplicity)
		 *
		 * IS[ t, j ] = sum( RW[ j, i ] * IS[ t-1, i ] ) ; i[ 0, number of hidden units ]
		 *
		 * This only takes the recurrent state into account. The input is calculated in the
		 * previous step.
		 *
		 */

		bool useConvertedWeights = this->mConvertedWeights.size() < 0;
		TensorP<ConvertedWeights> cRecurrentWeights;
		if( useConvertedWeights )
			cRecurrentWeights = this->mConvertedWeights[ 2 ];


		for ( uint batchIdx = 0; batchIdx < this->mInput->shape[ 0 ]; ++batchIdx ) { // iterate over the batch
			if ( timeIdx > 0 ) { // currently we dont have an inital state so just skip it for the first timestep
				for ( uint inIdx = 0; inIdx < mUnits; ++inIdx ) { // iterate over the incoming recurrent state
					WeightType w = ( *this->mRecurrentWeights )[ { unitIdx, inIdx } ];
					// if w is 0 we can just skip the step because it will add nothing
					if ( w == 0 )
						continue;
					ValueType temp = this->mInput->empty(); //TODO this could be done more efficently if we did not need to create empties
					if ( mReturnSquences )
						temp += ( *innerStates )[ { batchIdx, timeIdx - 1, inIdx } ];
					else
						temp += ( *lastInnerStates )[ { batchIdx, inIdx } ];
						

					if( useConvertedWeights )
						temp *= ( *cRecurrentWeights )[ { unitIdx, inIdx } ];
					else
						temp *= w;
					// if we are in the last time step and are not returning the sequences
					// write to the layer output instead
					if ( mReturnSquences  )
						( *innerStates )[ { batchIdx, timeIdx, unitIdx } ] += temp;
					else
						( *innerStates )[ { batchIdx, unitIdx } ] += temp;
				}
			}
		}
	}

	/**
	 * @brief Add bias and apply activation for one unit
	 */
	void activation( uint unitIdx, uint timeIdx ){
		bool useConvertedWeights = this->mConvertedWeights.size() < 0;
		TensorP<ConvertedWeights> cBiases;
		if( useConvertedWeights )
			cBiases = this->mConvertedWeights[ 1 ];

		for ( uint batchIdx = 0; batchIdx < this->mInput->shape[ 0 ]; ++batchIdx ) { // iterate over the batch
			if ( mReturnSquences ) {
				if( useConvertedWeights )
					( *innerStates )[ { batchIdx, timeIdx, unitIdx } ] += ( *cBiases )[ { unitIdx } ];
				else
					( *innerStates )[ { batchIdx, timeIdx, unitIdx } ] += ( *this->mBiases )[ { unitIdx } ];
				this->mActivation->activate( ( *innerStates )[ { batchIdx, timeIdx, unitIdx } ] );
			}
			else {
				if( useConvertedWeights )
					( *innerStates )[ { batchIdx, unitIdx } ] += ( *cBiases )[ { unitIdx } ];
				else
					( *innerStates )[ { batchIdx, unitIdx } ] += ( *this->mBiases )[ { unitIdx } ];
				this->mActivation->activate( ( *innerStates )[ { batchIdx, unitIdx } ] );
			}
		}
	}

	/**
	 * @brief Add bias and apply activation to all units which indexes are in
	 * in idxs
	 */
	void activations( std::vector<uint> idxs, uint timeIdx ){
		for( auto i : idxs )
			activation( i,timeIdx );
	}


};

//TODO remmove activation function

template<class ValueType, class WeightType, class DataTensorType, class WeightTensorType>
class AveragePooling: public Layer<ValueType, WeightType, DataTensorType, WeightTensorType> {
public:

	AveragePooling( std::string name, ActivationP<ValueType> act, uint filterSize, uint stride,
			PADDING_MODE pad, TensorP<ValueType> input, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input, dataTensorFactory, weigthTensorFactory ),
					mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	AveragePooling( std::string name, ActivationP<ValueType> act, uint filterSize, uint stride,
			PADDING_MODE pad, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			:
					Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, dataTensorFactory,
							weigthTensorFactory ),
					mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {

	}

	AveragePooling( std::string name, ActivationP<ValueType> act, uint filterSize, uint stride,	PADDING_MODE pad, TensorP<ValueType> input)
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act, input  ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	AveragePooling( std::string name, ActivationP<ValueType> act, uint filterSize, uint stride,
			PADDING_MODE pad  ) : Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, act ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
	}

	AveragePooling( std::string name, uint filterSize, uint stride,
			PADDING_MODE pad ) : Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer() ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
	}

	AveragePooling( std::string name, uint filterSize, uint stride, PADDING_MODE pad, TensorP<ValueType> input )
		: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer(), input ), mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
		}


	//new
	AveragePooling( std::string name, ActivationP<ValueType> act,
			uint filterSize, uint stride, PADDING_MODE pad,
			TensorP<ValueType> input,
			TensorFactoryP<ValueType> dataTensorFactory,
			TensorFactoryP<WeightType> weightTensorFactory ) :
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name, act, input,
							dataTensorFactory, weightTensorFactory ),
					mFilterSize( filterSize ), mStride( stride ), mPad( pad ) {
		this->mOutput = this->mDTF.get()->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	AveragePooling( std::string name, ActivationP<ValueType> act,
			uint filterSize, uint stride, PADDING_MODE pad,
			TensorFactoryP<ValueType> dataTensorFactory,
			TensorFactoryP<WeightType> weightTensorFactory ) :
					Layer<ValueType, WeightType, DataTensorType,
							WeightTensorType>( name, act, dataTensorFactory,
							weightTensorFactory ), mFilterSize( filterSize ),
					mStride( stride ), mPad( pad ) {
	}

	// computes the outputshape for the layer
	Shape outputShape() override {
		uint outputSizeX, outputSizeY;
		if ( mPad == SAME ) {
			// x direction
			assert( mStride != 0 && "Do not want to mod 0" );
			assert( this->mInput->shape.size >= 4 );

			if ( this->mInput->shape[ 3 ] % mStride == 0 ) {
				assert( mFilterSize - mStride >= 0 );
			} else {
				assert( ( mFilterSize - ( this->mInput->shape[ 3 ] % mStride ) ) >= 0 );
			}
			// y direction
			if ( this->mInput->shape[ 2 ] % mStride == 0 ) {
				assert( mFilterSize - mStride >= 0 );
			} else {
				assert( ( mFilterSize - ( this->mInput->shape[ 2 ] % mStride ) ) >= 0 );
			}
			outputSizeX = std::ceil( this->mInput->shape[ 3 ] / ( mStride * 1.0 ) );
			outputSizeY = std::ceil( this->mInput->shape[ 2 ] / ( mStride * 1.0 ) );
		} else {
			outputSizeX = std::ceil( ( this->mInput->shape[ 3 ] - mFilterSize + 1 ) / ( mStride * 1.0 ) );
			outputSizeY = std::ceil( ( this->mInput->shape[ 2 ] - mFilterSize + 1 ) / ( mStride * 1.0 ) );
		}
		return Shape { this->mInput->shape[ 0 ], this->mInput->shape[ 1 ], outputSizeX, outputSizeY };
	}

	void feedForward() override {
		std::thread tt[ POOL_SIZE ];
		uint j = 0;
		while ( j < this->mInput->shape[ 1 ] ) {
			int noThreads = 0;
			for ( unsigned int i = 0; i < POOL_SIZE; i++ ) {
				tt[ i ] = std::thread( [=] {this->pool( j );} );
				j++;
				noThreads = i + 1;
				if ( j == this->mInput->shape[ 1 ] )
					break;
			}
			for ( int i = 0; i < noThreads; i++ )
				tt[ i ].join();
		}
	}

	void loadWeights( std::string path, std::string fileName ) override {
		return;
	}

	void loadWeights( std::string path ) override {
		return;
	}

	void randomInit() override {
		return;
	}

	void description() override {
		std::cout << this->mName << "\t" << this->mOutput->shape << std::endl;
	}

	/**
	 * @brief Returns empyt vector
	 */
	virtual std::vector<TensorP<WeightType>> allWeights() override {
		return std::vector<TensorP<WeightType>>();
	}

private:
	uint mFilterSize, mStride;
	PADDING_MODE mPad;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"

	//Since we use tensorflow as the backend for keras, we had to modify the pooling algorithm
	//tensorflow averages the pool of a nxn 'kernel' using same padding by averaging over the count
	//of actual values added to the sum.
	//e.g.: 2x2 kernel, [1][2] but 1, 2, and 3, are 'padded' numbers (thus 0)
	//                  [3][4]
	//      with regular average pooling, the average would be (0 + 0 + 0 + 4) / 4 = 1
	//      but through tensorflow's implm, the average is (4) / 1 = 4
	void pool( uint sequence ) {
		if ( mPad == PADDING_MODE::SAME ) {
			//determine padding amount
			int pady, padTop, padLeft;
			if ( this->mInput->shape [ 3 ] % mStride == 0 )
				pady = std::max<int>( mFilterSize - mStride, 0 );
			else
				pady = std::max<int>(
						mFilterSize - ( this->mInput->shape [ 2 ] % mStride ),
						0 );
			padLeft = padTop = pady / 2; 		// because symmetry

			//iterate over each batch
			for ( uint batchIdx = 0; batchIdx < this->mInput->shape [ 0 ]; batchIdx++ ) { // we need to makes some changes here for SIMD
				int outIdx = 0;
				uint outX = 0, outY = 0;
				//maintains counter for numbers summed since keras/tensorflow average over the numbers used and not the filterSize*filterSize
				std::vector<std::vector<int>> uses( this->mOutput->shape [ 2 ],
						std::vector<int>( this->mOutput->shape [ 3 ], 0 ) );

				//shift image based on padding
				for ( int y = 0; y < (signed) this->mInput->shape [ 2 ]; y +=
						mStride ) { // iterating over the image rows
					for ( int x = 0; x < (signed) this->mInput->shape [ 3 ];
							x += mStride ) { // iterating over the image columns
						for ( unsigned int filtery = 0; filtery < mFilterSize;
								filtery++ ) { //iter over filter cols
							int iy = y + filtery - padTop;
							for ( unsigned int filterx = 0;
									filterx < mFilterSize;
									filterx++ ) { //iter over filter rows
								int ix = x + filterx - padLeft;

								//we ignore any padded image area since n + 0 = n
								if ( ix >= 0 && ix < (signed) this->mInput->shape [ 3 ]	&& iy >= 0 && iy < (signed) this->mInput->shape [ 2 ] ) {
									ValueType value = this->mInput->empty();
									value += ( *this->mInput ) [ { batchIdx, sequence, iy, ix } ];
									( *this->mOutput ) [ { batchIdx, sequence, outY, outX } ] += value;
									//TODO: do we have to use empty()? Maybe for HE but even then, value should be the same?
									//keep track of how many numbers have been summed
									uses [ outY ] [ outX ] += 1;
								}
							}
						}

						outIdx++;
						// calculate the indices for the output pixels
						outY = outIdx / this->mOutput->shape [ 2 ];
						outX = outIdx % this->mOutput->shape [ 3 ];
					}
				}
				//FIXME this will be way cooler with slicing
				for ( int y = 0; y < (signed) this->mOutput->shape [ 2 ]; ++y ) {
					for ( int x = 0; x < (signed) this->mOutput->shape [ 3 ]; ++x ) {
						assert( uses [ y ] [ x ] != 0 );//do not want to divide by 0
						( *this->mOutput ) [ { batchIdx, sequence, y, x } ] *=
								1.0 / uses [ y ] [ x ]; //average over the amount of numbers used to evaluate
						//TODO: this wont work for HE b.c. division not available
					}
				}
			}
		}
		else {						// valid padding is used
			//go through each batch
			for ( unsigned int batchIdx = 0; batchIdx < this->mInput->shape [ 0 ]; batchIdx++ ) { // we need to makes some changes here for SIMD
				int outIdx = 0;
				uint outX = 0, outY = 0;
				for ( int y = 0; y < (signed) this->mInput->shape [ 2 ]; y += mStride ) { // iterating over the image rows
					for ( int x = 0; x < (signed) this->mInput->shape [ 3 ]; x += mStride ) { // iterating over the image columns
						bool validFilter = true; // to break out of inner filter loop
						ValueType weightedSum = this->mInput->empty();

						for ( unsigned int filtery = 0; filtery < mFilterSize; filtery++ ) { //iter over filter cols
							int iy = y + filtery;

							//check if we are in valid filter area
							if ( iy < 0	|| iy >= (signed) this->mInput->shape [ 2 ]	|| !validFilter ) {
								validFilter = false;
								break;
							}
							for ( unsigned int filterx = 0;	filterx < mFilterSize; filterx++ ) { //iter over filter rows
								int ix = x + filterx;

								//check if we are in valid filter area
								if ( ix < 0	|| ix >= (signed) this->mInput->shape [ 3 ] || !validFilter ) {
									validFilter = false;
									break;
								}
								weightedSum += ( *this->mInput ) [ { batchIdx,
										sequence, iy, ix } ];
							}
						}
						if ( validFilter ) {
							( *this->mOutput ) [ { batchIdx, sequence, outY,
									outX } ] += weightedSum;
							outIdx++;
							// calculate the indices for the output pixels
							outY = outIdx / this->mOutput->shape [ 2 ];
							outX = outIdx % this->mOutput->shape [ 3 ];
						}
					}
				}
				//FIXME this will be way cooler with slicing
				for ( int y = 0; y < (signed) this->mOutput->shape [ 2 ]; ++y ) {
					for ( int x = 0; x < (signed) this->mOutput->shape [ 3 ]; ++x ) {
						( *this->mOutput ) [ { batchIdx, sequence, y, x } ] *=
								1.0 / (float) ( mFilterSize * mFilterSize ); /// average
					}
				}
			}
		}
	}
#pragma GCC diagnostic pop

};

template<class ValueType, class WeightType, class DataTensorType=TensorP<ValueType>, class WeightTensorType=TensorP<WeightType>, class ConvertedWeights=float>
class ZeroPadding2D: public Layer<ValueType, WeightType, DataTensorType, WeightTensorType> {
public:

	ZeroPadding2D( std::string name, uint padding, TensorP<ValueType> input, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer(), input, dataTensorFactory, weigthTensorFactory ), mPadding(padding )
	{
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	ZeroPadding2D( std::string name, uint padding, TensorFactory<ValueType>* dataTensorFactory, TensorFactory<WeightType>* weigthTensorFactory )
			:
	Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer(), dataTensorFactory, weigthTensorFactory ), mPadding(padding )
	{
	}

	ZeroPadding2D( std::string name, uint padding, TensorP<ValueType> input )
			: Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer(), input), mPadding(padding )
	{
		this->mOutput = this->mDataTensorFactory->create( outputShape() );
	}

	// This constructor is to be used when the layer is passed to the model and is not the first layer
	ZeroPadding2D( std::string name, uint padding)
			:
	Layer<ValueType, WeightType, DataTensorType, WeightTensorType>( name, LinearActivation<ValueType>::getSharedPointer() ), mPadding(padding )
	{
	}



	// computes the outputshape for the layer
	Shape outputShape() override {
		// TODO check for correct dims
		uint dim = 1;
		for ( uint i = 1; i < this->mInput->shape.size; ++i )
			dim *= this->mInput->shape[ i ];
		return Shape( { this->mInput->shape [ 0 ], this->mInput->shape [ 1 ], this->mInput->shape [ 2 ] + 2*mPadding, this->mInput->shape [ 3 ] + 2*mPadding } ) ;
	}

	void feedForward() override {
		this->mOutput->init();
		for (uint b = 0; b < this->mInput->shape [ 0 ]; ++b) { //batch
			for (uint c = 0; c < this->mInput->shape [ 1 ]; ++c) { // channel
				for (uint y = 0; y < this->mInput->shape [ 2 ]; ++y) { // y
					for (uint x = 0; x < this->mInput->shape [ 3 ]; ++x) { // x
						( *this->mOutput ) [ { b, c, y + mPadding, x + mPadding } ] = ( *this->mInput ) [ { b, c, y, x } ];
					}
				}
			}
		}
	}


	void loadWeights( std::string path, std::string fileName ) {
		// nothing to do
	}

	void loadWeights( std::string path ) {
		// nothing to do
	}
	virtual void randomInit() override {
		// nothing to do
	}

	void description() override {
		std::cout << this->mName << std::endl;
		//TODO: other info such as shape, size, etc.
	}

	/**
	 * @brief Returns empty vector
	 */
	virtual std::vector<TensorP<WeightType>> allWeights() override {
		return std::vector<TensorP<WeightType>>();
	}

private:
	uint mPadding;

};


#endif /* ARCHITECTURE_LAYERIMPL_H_ */
