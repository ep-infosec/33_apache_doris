// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

package org.apache.doris.statistics;

import org.apache.doris.common.CheckedMath;

import com.google.common.base.Preconditions;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * Derive NestedLoopJoinNode statistics.
 */
public class NestedLoopJoinStatsDerive extends BaseStatsDerive {
    private static final Logger LOG = LogManager.getLogger(NestedLoopJoinStatsDerive.class);

    @Override
    protected long deriveRowCount() {
        Preconditions.checkState(childrenStatsResult.size() == 2);
        if (childrenStatsResult.get(0).getRowCount() == -1 || childrenStatsResult.get(1).getRowCount() == -1) {
            rowCount = -1;
        } else {
            rowCount = CheckedMath.checkedMultiply((long) childrenStatsResult.get(0).getRowCount(),
                    (long) childrenStatsResult.get(1).getRowCount());
            applyConjunctsSelectivity();
            capRowCountAtLimit();
        }
        if (LOG.isDebugEnabled()) {
            LOG.debug("stats CrossJoin: rowCount={}", Double.toString(rowCount));
        }
        return rowCount;
    }
}
