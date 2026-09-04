# M1600A-S2 Full-Scale Source And Router Report

## Decision

**Stop the M1600A balanced query-router branch.** Scaling from 4,000/500 to
10,000/1,000 qrels-free rows strengthened the frozen posting source and kept a
near-perfect source oracle, but it did not amplify the trained router gain.
The run fails the predeclared continuation threshold, so official qrels and
cross-corpus evaluation were not opened.

## Locked Surface

- Dense root: `BAAI/bge-base-en-v1.5`.
- Train: 10,000 MS MARCO query rows and 88,992 unique documents.
- Heldout: 1,000 disjoint query rows and 8,988 unique documents.
- Posting namespace: 8 groups x 512 keys; one document key per group for the
  router.
- Query router: rank-64 residual; fixed 0.15x read cap; 1,000 steps; seed
  1602.
- Teacher: greedy dense-top256 key utility, head weight 10, tail weight 1,
  exact posting-length cost.
- Qrels, cross-encoder labels, BM25, dataset identity, and per-row policy:
  unused.

## Frozen Source Capacity

| Doc keys | Query policy | O@100 | O@256 | Reads | Max DF |
| ---: | --- | ---: | ---: | ---: | ---: |
| 1 | g8/p4 | 0.704770 | 0.546797 | 0.076216x | 0.009457 |
| 1 | g8/p8 | 0.831010 | 0.704906 | 0.146086x | 0.009457 |
| 2 | g8/p4 | 0.830200 | 0.701934 | 0.154322x | 0.016133 |
| 2 | g8/p8 | 0.919300 | 0.837445 | 0.299274x | 0.016133 |

The one-key source oracle remains dense-equivalent:

| Read cap | O@100 | O@256 | Actual reads | Candidate union |
| ---: | ---: | ---: | ---: | ---: |
| 0.075x | 0.998760 | 0.788547 | 0.074686x | 0.065068 |
| 0.150x | 1.000000 | 0.992906 | 0.130539x | 0.109908 |
| 0.300x | 1.000000 | 0.999996 | 0.134401x | 0.113021 |

The larger corpus therefore does not remove source capacity. It increases the
gap between available keys and the nearest-code query policy.

## Full-Scale Router

ClearML task: `cef67d5599534f3a898f559333c8edfc`.

| Surface | O@10 | O@100 | O@256 | Reads | Candidate union |
| --- | ---: | ---: | ---: | ---: | ---: |
| frozen base | 0.953000 | 0.822290 | 0.707828 | 0.149978x | 0.110795 |
| dense-key teacher | 1.000000 | 1.000000 | 0.992867 | 0.149978x | 0.127599 |
| selected step 200 | 0.952800 | 0.824180 | 0.708773 | 0.149978x | 0.109785 |
| step 1000 | n/a | 0.727200 | 0.619363 | 0.149978x | n/a |

Selected deltas versus the frozen base:

- O@10: `-0.000200`;
- O@100: `+0.001890`;
- O@256: `+0.000945`;
- reads: unchanged.

The router captures only 1.06% of the teacher O@100 gap and 0.33% of the
teacher O@256 gap. Continued optimization lowers the training objective while
destroying heldout hard overlap.

## Predeclared Gate

| Requirement | Observed | Pass |
| --- | ---: | --- |
| delta O@100 >= +0.01 | +0.001890 | no |
| delta O@256 >= +0.005 | +0.000945 | no |
| O@10 delta >= -0.001 | -0.000200 | yes |
| trained checkpoint selected | step 200 | yes |

The conjunctive gate fails. The branch therefore does not run FiQA, NFCorpus,
or SciFact. This is a mechanism stop, not an infrastructure or compute stop.

## Artifacts

- Run root:
  `/home/huoju/leask/runs/ii42-m1600-balanced-discrete-v1`.
- Source codebook SHA-256:
  `b3adfaf691a5a5ae68920c7065f731b663dadda33fc08889a976c3f8729ec944`.
- Source summary SHA-256:
  `7d534673b7bc14bacb0b93eed8242bc6f2fb3a78154f4ba713d1f3fb668a4939`.
- Selected router SHA-256:
  `3f650d2f11fb93f4aae94cec309aefa4565967281da93cac71f895dea50d1c6e`.
- Router summary SHA-256:
  `1589597737db733a306fadc11f63633370f0b73454d76681ea0e1fdac3d29ff8`.

## Conclusion

The source can hold the desired dense neighborhood, but the useful key order
depends on corpus posting lengths and the query's exact dense top256. A static
low-rank query residual does not recover that contextual combinatorial
function. Two seeds prove that an early gradient signal exists; the larger
surface proves that more examples and more steps do not turn it into a useful
router.

Do not continue with rank, loss-weight, threshold, or depth sweeps. A future
mechanism must make useful posting access locally observable from encoded text
or change source construction so that nearest query keys are useful by
design. It cannot retain this greedy corpus-context teacher and merely train a
larger selector.
