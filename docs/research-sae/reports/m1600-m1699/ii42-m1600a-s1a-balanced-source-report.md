# M1600A-S1A Balanced Source Report

## Decision

**Reject the first one-key-per-group joint-training checkpoint family.** No
trained checkpoint improved both hard dense O@100 and O@256 over the qrels-free
initialization, so the run correctly selected no trained model and did not
read official FiQA qrels.

This result does not yet close M1600. It authorizes the single structural
repair already allowed by the M1600 contract: measure a finer, document
multi-assignment source and separate source capacity from query routing before
any more training.

## Locked Run

- ClearML task: `28749502c7bb4371b6134f4230d569af`.
- Dense root: `BAAI/bge-base-en-v1.5`.
- Training surface: 4,000 MS MARCO queries and 35,831 unique documents.
- Validation surface: 500 disjoint queries and 4,498 unique documents.
- Cross-encoder labels and qrels: not used.
- Dense teacher candidates: 16 head documents, 80 samples spanning the
  remainder of dense top256, and 32 documents outside dense top256.
- Posting source: 8 groups, 128 keys per group, one document key per group.
- Checkpoint policy: six query groups and one key per selected group.
- Mean initial reads: 0.058557x.

## Qrels-Free Result

| Checkpoint | O@10 | O@100 | O@256 | Reads | Max DF |
| --- | ---: | ---: | ---: | ---: | ---: |
| initialization | 0.831600 | 0.519079 | 0.428560 | 0.058557x | 0.024678 |
| step 50 | 0.821600 | 0.513771 | 0.424902 | 0.057842x | 0.024455 |
| step 200 | 0.808800 | 0.493431 | 0.404545 | 0.056161x | 0.025122 |
| step 400 | 0.807200 | 0.480993 | 0.384516 | 0.057562x | 0.024678 |

The broader initialization frontier confirms that extra reads help but do not
repair the source:

| Policy | O@100 | O@256 | Reads | Candidate union |
| --- | ---: | ---: | ---: | ---: |
| g4/p1 | 0.442892 | 0.400671 | 0.039305x | 0.029408 |
| g6/p1 | 0.519079 | 0.428560 | 0.058557x | 0.040653 |
| g8/p1 | 0.574360 | 0.449403 | 0.077351x | 0.051425 |
| g8/p2 | 0.725640 | 0.573242 | 0.147114x | 0.091945 |

## Failure Mechanics

The negative is stronger than a generic optimization failure:

1. The initial source lacks dense-neighborhood capacity. At a read cost already
   above M1565 route2048, O@100 and O@256 are far lower than M1565's 0.906235
   and 0.842683.
2. Training reduced some DF concentration but did not create useful
   collisions. The validation max DF remained around 2.2-2.5%, while both
   overlap metrics declined at every checkpoint.
3. The shared geometry adapter moved increasingly far from its root
   (`adapter` diagnostic approximately 0.21 at step 400), while hard candidate
   coverage deteriorated. This is the exact failure the conjunctive gate was
   designed to reject.
4. Lower batch loss is not a usable selection signal. Listwise loss and hard
   match statistics fluctuated while the full heldout posting union declined
   monotonically.

The result agrees with RepCONC's warning that unconstrained joint clustering
can produce unstable assignments, and with Distill-VQ's finding that ranking
fit alone does not establish a useful index source.

## Authorized Next Probe

M1600A-S1B is a qrels-free source-capacity audit, not another training run:

- increase the vocabulary to 8 x 512 keys;
- test one and two document assignments per group;
- test bounded query multi-probe policies;
- compare deterministic query routing against a dense-teacher greedy source
  oracle at 0.075x, 0.15x, and 0.30x read caps.

If the source oracle fails O@100 0.95 and O@256 0.90, stop the grouped
multi-code source. If the oracle passes but deterministic routing fails, the
only justified continuation is a fixed-document, balanced query-router stage
using constrained assignments. Free joint training is not authorized.
