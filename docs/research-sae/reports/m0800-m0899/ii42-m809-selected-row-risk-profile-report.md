# M809 Selected Row Risk Profile

M809 profiles selected rows from the M807 mixed selector.  It compares
rows selected inside task-level negative tasks against rows selected
inside non-negative tasks.

## Summary

| Held-out | Lambda | Negative Tasks | Risk Rows | Risk Harmful | Risk p_harm | Safe Rows | Safe Harmful | Safe p_harm |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 3.0 | None | 0 | 0.000 | 0.000 | 48 | 0.042 | 0.127 |
| seed7642 | 3.0 | dbpedia-entity | 1 | 1.000 | 0.108 | 43 | 0.000 | 0.100 |
| seed7643 | 3.0 | fiqa, msmarco | 6 | 0.333 | 0.120 | 25 | 0.000 | 0.126 |
| original | 5.0 | None | 0 | 0.000 | 0.000 | 51 | 0.000 | 0.112 |
| seed7642 | 5.0 | climate-fever, dbpedia-entity | 8 | 0.250 | 0.129 | 40 | 0.000 | 0.082 |
| seed7643 | 5.0 | fiqa, msmarco | 6 | 0.333 | 0.117 | 34 | 0.000 | 0.115 |

## Decision

M809 shows negative-task selected rows are often already query-level harmful. A stricter harmful-head cap is plausible.
