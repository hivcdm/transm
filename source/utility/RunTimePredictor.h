#pragma once

#include <numeric>

class RunTimePredictor
{
public:
    RunTimePredictor() 
        : a_(0),
        b_(0),
        total_months_(0),
        estimated_percent_complete_(0),
        elapsed_time_(0),
        sx2y_(0),
        sylny_(0),
        sxy_(0),
        sxylny_(0),
        sy_(0),
        observations_(0)
    {
    }

    RunTimePredictor &operator=(const RunTimePredictor &) = delete;

    void SetTotalMonths(int months)
    {
        total_months_ = months;
    }

    void Update(const std::pair<int, double> &observation)
    {
        observations_++;
        elapsed_time_ += observation.second;

        if(observations_ < 5)
        {
            pop_average(observation);
            return;
        }

        auto average = pop_average(observation);
        
        sx2y_ += average.first * average.first * average.second;
        sylny_ += average.second * std::log(average.second);
        sxy_ += average.first * average.second;
        sxylny_ += average.first * average.second * std::log(average.second);
        sy_ += average.second;

        double denominator = sy_ * sx2y_ - sxy_ * sxy_;

        if(denominator == 0)
        {
            return;
        }

        a_ = std::exp((sx2y_ * sylny_ - sxy_ * sxylny_) / denominator);
        b_ = 0.0022; //std::max(0.0001, std::min(0.004, (sy_ * sxylny_ - sxy_ * sylny_) / denominator));
        auto f = [=](double x) { return a_ * std::exp(b_ * x); };
        auto integral = [=](double a, double b) { return f(b) - f(a); };
        double current_area = integral(1, observation.first);
        double total_area = integral(1, total_months_);
        estimated_percent_complete_ = current_area / total_area;
    }

    double GetEstimatedPercentComplete() const
    {
        return estimated_percent_complete_;
    }

    double GetEstimatedTimeRemaining() const
    {
        return GetEstimatedRunTime() - elapsed_time_;
    }

    double GetEstimatedRunTime() const
    {
        return estimated_percent_complete_ == 0 ? 0 : elapsed_time_ / estimated_percent_complete_;
    }

    std::string MakeProgressBar(std::size_t bar_length) const
    {
        std::size_t bars_completed = (int)(GetEstimatedPercentComplete() * bar_length);
        std::size_t bars_remaining = bar_length - bars_completed;
        std::string bar = "[" + std::string(bars_completed, '=') + "|" + std::string(bars_remaining, '=') + "]";
        return bar;
    }

    std::pair<double, double> pop_average(const std::pair<int, double> next)
    {
        if(previous_.size() > 4)
        {
            previous_.pop_front();
        }

        previous_.push_back(next);

        double sum_y = 0;

        for(const auto &observation : previous_)
        {
            sum_y += observation.second;
        }

        return std::make_pair(next.first, sum_y / previous_.size());
    }

private:
    double a_;
    double b_;
    int total_months_;
    double estimated_percent_complete_;
//    int estimated_final_pop_size_;
    double elapsed_time_;
    std::deque<std::pair<int, double>> previous_;
    double sx2y_;
    double sylny_;
    double sxy_;
    double sxylny_;
    double sy_;
    int observations_;
};
