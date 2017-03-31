/**
 * DeepDetect
 * Copyright (c) 2014 Emmanuel Benazera
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

#include "deepdetect.h"
#include "commandlineapi.h"
#include "commandlinejsonapi.h"
#include "httpjsonapi.h"
#include "dd_config.h"
#include "githash.h"

namespace dd
{
  template<class TAPIStrategy>
  std::string DeepDetect<TAPIStrategy>::_commit_version = GIT_COMMIT_HASH;

  template<class TAPIStrategy>
  DeepDetect<TAPIStrategy>::DeepDetect()
  {
    std::cout << "DeepDetect [ commit " << DeepDetect::_commit_version << " ]\n";
  }

  template<class TAPIStrategy>
  DeepDetect<TAPIStrategy>::~DeepDetect()
  {
  }

  template class DeepDetect<CommandLineAPI>;
  template class DeepDetect<CommandLineJsonAPI>;
  template class DeepDetect<HttpJsonAPI>;
}
