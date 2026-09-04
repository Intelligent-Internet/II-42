# M794 Context-Bundle Proposal Generator

M794 changes the proposal surface: instead of selecting from a fixed
global M779 bundle pool, it builds small same-direction bundles per
query from qrels-free native-context single-coordinate attempts.

## Test Results

| Score | Gate | Clean | Applied | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| margin_bundle | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| score_mean_margin | 1 | 1 | 0.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| conservative_margin | 0 | 0 | 10.3 | +0.000003 | -0.000004 | +0.000000 | +0.000000 | +0.000003 | +0.000000 | -0.000000 | seed7643:dbpedia-entity,trec-covid |

## Decision

M794 is clean but diagnostic-scale only. Keep query-local bundle generation as evidence, not a promoted route.
