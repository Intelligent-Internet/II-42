# M1571 Overlapping Cosine-LSH Posting Report

## Decision

**Stop random-hyperplane buckets as a single-hop posting source.**

The theoretical collision mechanism is strongly visible, but the corpus is
too anisotropic for the analytically expected bucket load. A few exact query
buckets fill the unique candidate budget before evidence from other tables can
accumulate. The run fails the predeclared maximum-DF integrity gate and every
retrieval-capacity gate.

Do not change the seed, bits, table count, radius, or probe order. Do not train
hash functions. The only retained signal is for one separately contracted
index-internal multi-hit accumulation audit; it is not a continuation of the
failed first-hit source.

## Surface And Integrity

- Official FiQA: 57,638 documents and 648 queries.
- 16 tables, 12 random-hyperplane bits, radius-2 margin probing.
- Exactly 922,208 postings, or 16 per document.
- 25,613 non-empty keys in a 65,536-key namespace.
- Mean non-empty load `36.01`, p95 `138`, maximum `6,210`.
- Maximum DF ratio `0.107741`, above the `0.01` gate.
- Normalized load entropy `0.809882`.
- Qrels-free basis SHA-256:
  `f595bee2391268741a189e8cd924c3b558c8d62b33e72e2110daceedb23db340`.
- Qrels-free candidate-surface SHA-256:
  `1c41a753a33bf7267ae5bfce1acf76b94fbbc4b8d7b49efbc8d66bb27811246e`.
- ClearML task: `4f200f186f6e4846aca35952a6996f35`.
- Runtime: 24.6 seconds.

## Dense Collision Coverage

| Dense depth | Exact bucket | Radius <=1 | Radius <=2 |
| ---: | ---: | ---: | ---: |
| 10 | 0.397531 | 0.923611 | 0.998765 |
| 100 | 0.307917 | 0.867932 | 0.996620 |
| 256 | 0.277133 | 0.841110 | 0.994942 |

This is a real cosine-neighborhood signal. It does not imply a usable
candidate source because accessing the relevant buckets and selecting among
their members are separate requirements.

## First-Hit Frontier

| Budget | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB | Reads | Probes |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1,000 | 0.089660 | 0.062994 | 0.054579 | 0.066379 | 0.064052 | 0.050246 | 0.103028 | 0.069401 | 0.017426x | 5.16 |
| 1,256 | 0.110494 | 0.077716 | 0.067570 | 0.081157 | 0.078132 | 0.061788 | 0.124024 | 0.084180 | 0.021920x | 6.01 |
| 2,048 | 0.168981 | 0.121713 | 0.106463 | 0.126031 | 0.113860 | 0.089543 | 0.175208 | 0.130134 | 0.035926x | 8.53 |

## Failure Mechanics

The bit-count derivation assumed roughly uniform bucket occupancy. BGE's
uncentered embedding distribution violates that assumption: origin-centered
hyperplanes produce several very hot codes. Queries share the same anisotropy,
so their exact buckets are often precisely those hot lists.

The first-hit policy stops after only six mean probes at budget 1,256. It
therefore returns arbitrary early members of a few hot buckets, while the
radius-2 collision coverage is distributed across as many as 1,264 possible
table/bucket probes. More query keys or a larger unique budget would not fix
the missing cross-table evidence mechanism.

Centering, whitening, median offsets, or learned hyperplanes would change the
contract and remove the exact Charikar collision claim being tested. Those are
not authorized follow-ups.

## One Retained Controller Audit

The source-level stop does not answer whether fixed-budget accumulation over
the already frozen postings can expose the observed collision evidence. One
index-internal audit is justified:

1. freeze the M1571 basis, radius-2 margin probe order, and posting lists;
2. decode at most `0.30x` corpus postings, counting duplicates;
3. accumulate independent-table match count and probe-margin evidence per
   document;
4. select exactly 1,256 candidates only after accumulation;
5. compare with first-hit M1571 and M1565 route1000.

This is a multi-stage inverted-index controller, not a new source or learned
selector. If it fails the dense-overlap gate, close LSH completely and move to
the product boundary: a protected-tail controller over the strongest existing
native unified source.
