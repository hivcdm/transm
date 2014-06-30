#pragma once

#pragma warning( push )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4244 )

#include <boost/numeric/ublas/blas.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>

class RunTimePredictor
{
public:
    RunTimePredictor(int degree = 2) 
        : degree_(degree + 1), 
        time_scale_(1 / 120.0),
        pop_scale_(1 / 1000.0),
        total_months_(0),
        coefficients_(degree_),
        estimated_percent_complete_(0), 
        estimated_final_pop_size_(0)
    {
    }

    RunTimePredictor &operator=(const RunTimePredictor &) = delete;

    void SetTotalMonths(int months)
    {
        total_months_ = months;
    }

    void Update(const std::vector<int> &population_size)
    {
        if(population_size.size() < degree_)
        {
            estimated_final_pop_size_ = population_size.front();
            estimated_percent_complete_ = population_size.size() / (double)total_months_;
            return;
        }

        std::vector<double> scaled_time(population_size.size());
        std::vector<double> scaled_population(population_size.size());
        int sum_pop = 0;

        for(std::size_t i = 0; i < population_size.size(); i++)
        {
            sum_pop += population_size[i];
            scaled_time[i] = i * time_scale_;
            scaled_population[i] = population_size[i] * pop_scale_;
        }

        coefficients_ = polyfit(scaled_time, scaled_population);

        double predicted_pop = sum_pop;
        for(int i = (int)population_size.size() + 1; i <= total_months_; i++)
        {
            predicted_pop += std::max(0.0, estimate_pop_size(i));
        }

        estimated_final_pop_size_ = (int)std::max(0.0, estimate_pop_size(total_months_));
        estimated_percent_complete_ = sum_pop / predicted_pop;
    }

    double GetEstimatedPercentComplete() const
    {
        return estimated_percent_complete_;
    }

    double GetEstimatedTimeRemaining(double time_elapsed) const
    {
        return GetEstimatedRunTime(time_elapsed) - time_elapsed;
    }

    double GetEstimatedRunTime(double time_elapsed) const
    {
        return time_elapsed / estimated_percent_complete_;
    }

    std::string MakeProgressBar(std::size_t bar_length) const
    {
        std::size_t bars_completed = (int)(GetEstimatedPercentComplete() * bar_length);
        std::size_t bars_remaining = bar_length - bars_completed;
        std::string bar = "[" + std::string(bars_completed, '=') + "|" + std::string(bars_remaining, '=') + "]";
        return bar;
    }

private:
    template<typename T>
    std::vector<T> polyfit(const std::vector<T>& oX, const std::vector<T>& oY)
    {
        using namespace boost::numeric::ublas;

        if(oX.size() != oY.size())
        {
            throw std::invalid_argument("X and Y vector sizes do not match");
        }

        size_t nCount = oX.size();
        matrix<T> oXMatrix(nCount, degree_);
        matrix<T> oYMatrix(nCount, 1);

        // copy y matrix
        for(size_t i = 0; i < nCount; i++)
        {
            oYMatrix(i, 0) = oY[i];
        }

        // create the X matrix
        for(size_t nRow = 0; nRow < nCount; nRow++)
        {
            T nVal = 1.0f;
            for(int nCol = 0; nCol < degree_; nCol++)
            {
                oXMatrix(nRow, nCol) = nVal;
                nVal *= oX[nRow];
            }
        }

        // transpose X matrix
        matrix<T> oXtMatrix(trans(oXMatrix));
        // multiply transposed X matrix with X matrix
        matrix<T> oXtXMatrix(prec_prod(oXtMatrix, oXMatrix));
        // multiply transposed X matrix with Y matrix
        matrix<T> oXtYMatrix(prec_prod(oXtMatrix, oYMatrix));

        // lu decomposition
        permutation_matrix<int> pert(oXtXMatrix.size1());
        const std::size_t singular = lu_factorize(oXtXMatrix, pert);
        // must be singular
        BOOST_ASSERT(singular == 0);

        // backsubstitution
        lu_substitute(oXtXMatrix, pert, oXtYMatrix);

        // copy the result to coeff
        return std::vector<T>(oXtYMatrix.data().begin(), oXtYMatrix.data().end());
    }

    double estimate_pop_size(int time)
    {
        double pop_size = 0;
        double nXT = 1;

        for(size_t j = 0; j < degree_; j++)
        {
            // multiply current x by a coefficient
            pop_size += coefficients_[j] * nXT;
            // power up the X
            nXT *= (time * time_scale_);
        }

        return pop_size / pop_scale_;
    }

    const int degree_;
    const double time_scale_;
    const double pop_scale_;
    int total_months_;
    std::vector<double> coefficients_;
    double estimated_percent_complete_;
    int estimated_final_pop_size_;
};

#pragma warning( pop )
