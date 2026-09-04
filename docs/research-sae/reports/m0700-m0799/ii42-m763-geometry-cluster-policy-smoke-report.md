# M763 Geometry-Cluster Policy Smoke

## Decision

Simple native-geometry clustering does not solve M758's task-level robustness
problem.

The smoke test clusters M758-selected rows using only inference-available
native-context features, then chooses which clusters to keep on dev under
strict no-negative-task constraints. Test replay still either becomes a no-op
or leaves negative datasets.

## Results

| Budget | Best Cluster Policy | Test Applied | Test Negative Tasks | Gate | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| top16 | k=4 keep=[1] | 110 | dbpedia-entity, msmarco, scidocs | 0 | +0.000011 | -0.000039 | -0.000185 | +0.000014 | +0.000000 |
| top32 | k=2 keep=[1] | 1 | none | 1 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| top64 | k=5 keep=[4] | 45 | dbpedia-entity | 1 | +0.000003 | +0.000069 | +0.000000 | +0.000000 | +0.000000 |

## Interpretation

The M762/M763 probes agree:

- global risk features are enough for macro safety;
- they are not enough for robust per-dataset/query safety;
- when the policy is made robust enough to remove all negative rows, useful
  gains collapse to near-zero.

This means the remaining risk is query-local and boundary-specific. It is not
well separated by static native geometry clusters or selected-row classifiers.

## Updated Route

The next useful experiment should not be another global classifier over the
same features. The next candidate is a cheap query-local native feedback step:

1. keep the M758 deterministic policy as the proposal generator;
2. before accepting a selected delta, compute a tiny local stability check on
   the affected boundary docs or top-k score margins;
3. accept only if the local check predicts no top-rank regression;
4. evaluate the accepted subset through the same native shared15 path.

If query-local feedback also collapses to no-op, then this line should stop as
a macro diagnostic rather than a deployable improvement.
