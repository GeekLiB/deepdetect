/**
 * DeepDetect
 * Copyright (c) 2014-2015 Emmanuel Benazera
 * Author: Emmanuel Benazera <beniz@droidnik.fr>
 *
 * This file is part of deepdetect.
 *
 * deepdetect is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * deepdetect is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with deepdetect.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MLLIBSTRATEGY_H
#define MLLIBSTRATEGY_H

#include "apidata.h"
#include "utils/fileops.hpp"
#include <atomic>
#include <exception>
#include <mutex>

namespace dd
{
  /**
   * \brief ML library bad parameter exception
   */
  class MLLibBadParamException : public std::exception
  {
  public:
    MLLibBadParamException(const std::string &s)
      :_s(s) {}
    ~MLLibBadParamException() {}
    const char* what() const noexcept { return _s.c_str(); }
  private:
    std::string _s;
  };
  
  /**
   * \brief ML library internal error exception
   */
  class MLLibInternalException : public std::exception
  {
  public:
    MLLibInternalException(const std::string &s)
      :_s(s) {}
    ~MLLibInternalException() {}
    const char* what() const noexcept { return _s.c_str(); }
  private:
    std::string _s;
  };

  /**
   * \brief main class for machine learning library encapsulation
   */
  template <class TInputConnectorStrategy, class TOutputConnectorStrategy, class TMLModel>
    class MLLib
  {
  public:
    /**
     * \brief constructor from model
     */
    MLLib(const TMLModel &mlmodel)
      :_mlmodel(mlmodel),_tjob_running(false) {}
    
    /**
     * \brief copy-constructor
     */
    MLLib(MLLib &&mll) noexcept
      :_inputc(mll._inputc),_outputc(mll._outputc),_mlmodel(mll._mlmodel),_meas(mll._meas),_tjob_running(mll._tjob_running.load())
      {}
    
    /**
     * \brief destructor
     */
    ~MLLib() {}

    /**
     * \brief initializes ML lib
     * @param ad data object for "parameters/mllib"
     */
    void init_mllib(const APIData &ad);

    /**
     * \brief clear the lib service from local model files etc...
     * @param ad root data object
     */
    void clear_mllib(const APIData &ad);

    /**
     * \brief removes everything in model repository
     */
    void clear_full()
    {
      int err = fileops::clear_directory(_mlmodel._repo);
      if (err > 0)
	throw MLLibBadParamException("Failed opening directory " + _mlmodel._repo + " for deleting files within");
      else if (err < 0)
	throw MLLibInternalException("Failed deleting all files in directory " + _mlmodel._repo);
    }

    /**
     * \brief train new model
     * @param ad root data object
     * @param out output data object (e.g. loss, ...)
     * @return 0 if OK, 1 otherwise
     */
    int train(const APIData &ad, APIData &out);

    /**
     * \brief predicts from model
     * @param ad root data object
     * @param out output data object (e.g. predictions, ...)
     * @return 0 if OK, 1 otherwise
     */
    int predict(const APIData &ad, APIData &out);
    
    /**
     * \brief ML library status
     */
    int status() const;
    
    /**
     * \brief clear all measures history
     */
    void clear_all_meas_per_iter()
    {
      std::lock_guard<std::mutex> lock(_meas_per_iter_mutex);
      _meas_per_iter.clear();
    }

    /**
     * \brief add value to measure history
     * @param meas measure name
     * @param l measure value
     */
    void add_meas_per_iter(const std::string &meas, const double &l)
    {
      std::lock_guard<std::mutex> lock(_meas_per_iter_mutex);
      auto hit = _meas_per_iter.find(meas);
      if (hit!=_meas_per_iter.end())
	(*hit).second.push_back(l);
      else
	{
	  std::vector<double> vmeas = {l};
	  _meas_per_iter.insert(std::pair<std::string,std::vector<double>>(meas,vmeas));
	}
    }

    /**
     * \brief collect current measures history into a data object
     * 
     */
    void collect_measures_history(APIData &ad)
    {
      APIData meas_hist;
      std::lock_guard<std::mutex> lock(_meas_per_iter_mutex);
      auto hit = _meas_per_iter.begin();
      while(hit!=_meas_per_iter.end())
	{
	  meas_hist.add((*hit).first+"_hist",(*hit).second);
	  ++hit;
	}
      ad.add("measure_hist",meas_hist);
    }

    /**
     * \brief sets current value of a measure
     * @param meas measure name
     * @param l measure value
     */
    void add_meas(const std::string &meas, const double &l)
    {
      std::lock_guard<std::mutex> lock(_meas_mutex);
      auto hit = _meas.find(meas);
      if (hit!=_meas.end())
	(*hit).second = l;
      else _meas.insert(std::pair<std::string,double>(meas,l));
    }

    /**
     * \brief get currentvalue of argument measure
     * @param meas measure name
     * @return current value of measure
     */
    double get_meas(const std::string &meas)
    {
      std::lock_guard<std::mutex> lock(_meas_mutex);
      auto hit = _meas.find(meas);
      if (hit!=_meas.end())
	return (*hit).second;
      else return std::numeric_limits<double>::quiet_NaN();
    }

    /**
     * \brief collect current measures into a data object
     * @param ad data object to hold the measures
     */
    void collect_measures(APIData &ad)
    {
      APIData meas;
      std::lock_guard<std::mutex> lock(_meas_mutex);
      auto hit = _meas.begin();
      while(hit!=_meas.end())
	{
	  meas.add((*hit).first,(*hit).second);
	  ++hit;
	}
      ad.add("measure",meas);
    }

    TInputConnectorStrategy _inputc; /**< input connector strategy for channeling data in. */
    TOutputConnectorStrategy _outputc; /**< output connector strategy for passing results back to API. */

    bool _has_train = false; /**< whether training is available. */
    bool _has_predict = true; /**< whether prediction is available. */

    TMLModel _mlmodel; /**< statistical model template. */
    std::string _libname; /**< ml lib name. */
    
    std::unordered_map<std::string,double> _meas; /**< model measures, used as a per service value. */
    std::unordered_map<std::string,std::vector<double>> _meas_per_iter; /**< model measures per iteration. */

    std::atomic<bool> _tjob_running = {false}; /**< whether a training job is running with this lib instance. */

    bool _online = false; /**< whether the algorithm is online, i.e. it interleaves training and prediction calls.
			     When not, prediction calls are rejected while training is running. */

  protected:
    std::mutex _meas_per_iter_mutex; /**< mutex over measures history. */
    std::mutex _meas_mutex; /** mutex around current measures. */
  };  
  
}

#endif
