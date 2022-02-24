//
// Created by administrator on 23.02.22.
//

#ifndef SRC_HASHING_H
#define SRC_HASHING_H

#define assertm(exp, msg) assert(((void)msg, exp))

#include <random>
#include "Eigen/Dense"
#include "Eigen/Sparse"
#include "Frame.h"
#include <cmath>
#include <algorithm>
#include <execution>


/**
 * check if a given key occurred in map more then once
 * @param keys
 * @param containsMap
 * @return
 */
bool containsKeyMultiple(const std::vector<int>& keys, const std::map<int, bool>& containsMap){
    bool moreThenOnce = false;
    for(const auto key: keys){
        if(containsMap.at(key)){
            moreThenOnce = true;
            break;
        }
    }
    return moreThenOnce;
}

/**
 * create eigen sparse vector from vectorized mz spectrum
 * @param mzVector : vectorized mz spectrum to convert
 * @param numRows : dimensionality of vector
 * @return : a sparse eigen vector suited for fast vectorized operations
 */
Eigen::SparseVector<double> toSparseVector(const MzVectorPL& mzVector, int numRows){

    Eigen::SparseMatrix<double> sparseVec = Eigen::SparseMatrix<double>(numRows, 1);
    std::vector<Eigen::Triplet<double>> tripletList;
    tripletList.reserve(mzVector.indices.size());

    for(std::size_t i = 0; i < mzVector.indices.size(); i++)
        tripletList.emplace_back(mzVector.indices[i], 0, mzVector.values[i]);

    sparseVec.setFromTriplets(tripletList.begin(), tripletList.end());
    return sparseVec;
}

/**
 * create a string representation of bits for easy hashing
 * @param boolVector vector to create bit string from
 * @return bit string
 */
std::string boolVectorToString(const std::vector<bool> &boolVector, int bin, bool restricted){

    std::string ret;

    for(const auto& b: boolVector)
        b ? ret.append("1") : ret.append("0");

    // hard restriction to its own mass bin only for collision
    if(restricted)
        ret.append(std::to_string(bin));
        // soft restriction to all windows with same offset
    else
        bin > 0 ? ret.append("1") : ret.append("0");

    return ret;
}

/**
 * calculate integer keys from vectors of boolean
 * @param hashes set of bool vectors representing the result from lsh probing
 * @return a set of ints representing the keyspace of a given mz spectrum or window
 */
std::vector<int> calculateKeys(const std::vector<std::vector<bool>> &hashes, int bin, bool restricted){
    std::vector<int> retVec;
    retVec.reserve(hashes.size());
    for(const auto& h: hashes){
        int hash = std::hash<std::string>{}(boolVectorToString(h, bin, restricted));
        retVec.push_back(hash);
    }
    return retVec;
}

/**
 *
 * @param sparseSpectrumVector
 * @param M random matrix of shape ((k * l) * mzSpace)
 * @param k number of ANDs
 * @param l number of ORs
 * @return k vectors of l bits
 */


std::vector<std::vector<bool>> calculateSignumVector(const Eigen::SparseVector<double>& sparseSpectrumVector,
                                                     const Eigen::MatrixXd& M,
                                                     int k,
                                                     int l){
    // check for compatible settings
    assertm(k * l == M.rows(), "dimensions of random vector matrix and banding differ.");

    std::vector<std::vector<bool>> retVec;
    retVec.reserve(k);

    // heavy lifting happens here, calculate dot products between random vectors and spectrum
    auto r = M * sparseSpectrumVector;

    // calculate signs from dot products
    std::vector<bool> bVec;
    bVec.reserve(l);

    for(std::size_t i = 0; i < r.size(); i++)
        bVec.push_back(r[i] > 0);

    // rest of the code is for grouping of results only
    std::vector<bool> leVec;
    leVec.reserve(l);

    for(std::size_t i = 0; i < bVec.size(); i++){
        if(i % l == 0 && i != 0){
            retVec.push_back(leVec);
            leVec.clear();
        }
        leVec.push_back(bVec[i]);
    }
    retVec.push_back(leVec);
    return retVec;
}

auto initMatrix = [](int k, int l, int seed, int res) -> Eigen::MatrixXd {

    std::default_random_engine generator(seed);
    std::normal_distribution<double> distribution(0, 1);
    auto normal = [&] (int) {return distribution(generator);};

    int resFactor = int(pow(10, res));

    return Eigen::MatrixXd::NullaryExpr(k * l, 2000 * resFactor, normal);
};


struct TimsHashGenerator {

    int seed, resolution, k, l;
    const Eigen::MatrixXd M;

    TimsHashGenerator(int kk, int ll, int s, int r): k(kk), l(ll), seed(s), resolution(r), M(initMatrix(kk, ll, s, r)){}

    const Eigen::MatrixXd &getMatrixCopy() { return M; }

    std::vector<std::pair<int, std::vector<int>>> hashSpectrum(MzSpectrumPL &spectrum,
                                                              int minPeaksPerWindow,
                                                              int minIntensity,
                                                              double  windowLength,
                                                              bool overlapping,
                                                              bool binRestricted);

};

std::vector<std::pair<int, std::vector<int>>> TimsHashGenerator::hashSpectrum(MzSpectrumPL &spectrum,
                                                                             int minPeaksPerWindow,
                                                                             int minIntensity,
                                                                             double windowLength,
                                                                             bool overlapping,
                                                                             bool binRestricted) {

    const auto windows = spectrum.windows(windowLength, overlapping, minPeaksPerWindow, minIntensity);
    std::vector<std::pair<int, std::vector<int>>> retVec;
    retVec.resize(windows.size());

    std::map<int, Eigen::SparseVector<double>> tmpVec;
    int numRows = int(2000 * pow(10, this->resolution));

    auto hashWindow = [&binRestricted, &numRows, this](std::pair<const int, MzSpectrumPL> p) -> std::pair<int, std::vector<int>> {
        auto sparseVec = toSparseVector(p.second.vectorize(this->resolution), numRows);
        auto signumVec = calculateSignumVector(sparseVec, this->M, this->k, this->l);
        auto keys = calculateKeys(signumVec, p.first, binRestricted);
        return {p.first, keys};
    };

    std::transform(std::execution::par_unseq, windows.begin(), windows.end(), retVec.begin(), hashWindow);
    return  retVec;
}


#endif //SRC_HASHING_H