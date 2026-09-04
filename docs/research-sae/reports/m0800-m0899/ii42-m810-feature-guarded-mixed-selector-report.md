# M810 Feature-Guarded Mixed Selector

M810 adds one qrels-free feature guard after the M807 mixed selector.
The guard is selected on non-held-out dev surfaces and replayed on
the held-out surface.

## Held-Out Cases

| Held-out | Lambda | Guard | Gate | Clean | Applied | dMAP | dNDCG | dMRR | Utility | Negative Tasks |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| original | 5.0 | source_score_min ge 0.5813 | 1 | 1 | 26.0 | +0.000004 | +0.000000 | +0.000000 | +0.000004 | None |
| seed7642 | 3.0 | none none 0.0000 | 1 | 0 | 44.0 | +0.000130 | +0.000113 | +0.000123 | +0.000268 | seed7642:dbpedia-entity |
| seed7642 | 5.0 | none none 0.0000 | 1 | 0 | 48.0 | +0.000123 | +0.000113 | +0.000123 | +0.000261 | seed7642:climate-fever;dbpedia-entity |
| seed7643 | 3.0 | none none 0.0000 | 1 | 0 | 31.0 | +0.000065 | +0.000066 | +0.000051 | +0.000141 | seed7643:fiqa;msmarco |
| seed7643 | 5.0 | none none 0.0000 | 1 | 0 | 40.0 | +0.000048 | +0.000066 | +0.000051 | +0.000124 | seed7643:fiqa;msmarco |
| original | 3.0 | source_score_min ge 0.5881 | 1 | 0 | 25.0 | +0.000003 | +0.000000 | +0.000000 | +0.000003 | original:climate-fever |

## Common Config Check

| Lambda | All Clean | Min Utility | Mean Utility | Applied Sum |
| ---: | ---: | ---: | ---: | ---: |
| 5.0 | 0 | +0.000004 | +0.000129 | 114.0 |
| 3.0 | 0 | +0.000003 | +0.000137 | 100.0 |

## Decision

M810 did not find a clean useful common config. Stop query-risk selector repair unless a stronger supervision source is added.
