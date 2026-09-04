# M1544 Clustered Coordinate-Block Audit

## Decision

**Close the pooled-dense inverted-posting equivalence route.**

M1544 tests the last untried access mechanism supported by the M1541 failure
and clustered inverted-index literature. Route-conditioned coordinate blocks
do not preserve enough dense candidates and introduce prohibitive block-summary
scan and storage cost.

Commit under test: `faeec864`.

ClearML task: `ffbd31d954ad46e79c25aa711fd8865f`.

## Result

| Dataset | Source | NDCG@10 | MAP@100 | Recall@100 | CUB | Dense O@100 | Reads | Union | Summary scans/query |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | exact BGE | 0.458739 | 0.232774 | 0.386586 | 0.812651 | 1.000000 | 1.0000x | 1.0000 | 0 |
| nfcorpus | block tail256 | 0.409187 | 0.190051 | 0.303293 | 0.418248 | 0.649200 | 0.1503x | 0.1503 | 12170.0 |
| scifact | exact BGE | 0.848783 | 0.812670 | 0.980000 | 1.000000 | 1.000000 | 1.0000x | 1.0000 | 0 |
| scifact | block tail256 | 0.816022 | 0.783978 | 0.900000 | 0.920000 | 0.722600 | 0.1500x | 0.1500 | 12608.1 |
| fiqa | exact BGE | 0.694897 | 0.641505 | 0.934806 | 0.996667 | 1.000000 | 1.0000x | 1.0000 | 0 |
| fiqa | block tail256 | 0.662758 | 0.603423 | 0.863504 | 0.881615 | 0.741800 | 0.1500x | 0.1500 | 12100.4 |
| macro | exact BGE | 0.667473 | 0.562316 | 0.767131 | 0.936439 | 1.000000 | 1.0000x | 1.0000 | 0 |
| macro | block dense upper | 0.625641 | 0.523791 | 0.692378 | 0.739955 | 0.705300 | 0.1501x | 0.1501 | 12292.8 |
| macro | block tail256 | 0.629322 | 0.525817 | 0.688932 | 0.739955 | 0.704533 | 0.1501x | 0.1501 | 12292.8 |

Gate result: `0/3`; NFCorpus violates the 90% row floor.

## Cost Failure

| Dataset | Blocks | Coordinate edges | FP16 summary bytes/doc |
| --- | ---: | ---: | ---: |
| nfcorpus | 72,411 | 264,064 | 53,913 |
| scifact | 75,669 | 256,000 | 58,114 |
| fiqa | 70,768 | 256,000 | 54,350 |

The implementation reads only 15% posting entries because most selected
blocks contain distinct documents. The expensive operation moves to summary
evaluation: about twelve thousand 768-dimensional summaries per query. This
is neither a compact posting index nor a competitive ANN replacement.

## Mechanism Conclusion

Across M1541-M1544:

1. full signed-coordinate accumulation plus tail256 reproduces dense ranking;
2. bounded individual-edge traversal loses nearly all useful candidates;
3. global corpus routes recover most retrieval quality at honest 15% union,
   but not dense-equivalent neighborhoods;
4. equal-budget lexical substitution does not add reliable capacity;
5. coordinate-local clustered blocks are worse than global corpus routes and
   have unacceptable summary cost.

This is sufficient evidence to stop more pooled-dense posting-head, route,
block, loss, epoch, and selector experiments. The bottleneck is not training
depth or the tail scorer. Dense geometry needs an ANN-shaped access structure
if dense equivalence is a hard requirement.

## One Remaining Retrieval-Only Check

M1543 identified why its fixed union failed: lexical candidates replaced
semantic candidates. A single additive check remains justified by both that
diagnosis and the older M550 rescue result:

- freeze M1542 dual route at 15%;
- add at most 5% unseen BM25 candidates, for a hard 20% total union;
- compare with dense and BM25 at the same 20% candidate budget;
- retain frozen P1 rank fusion; do not tune alpha or source allocation;
- authorize scorer training only if the additive union creates reliable
  candidate-capacity gain first.

This check is not allowed to reopen dense equivalence. It asks only whether a
bounded semantic+lexical retrieval source can exceed the individual sources.

## Artifacts

- `runs/m1544a_clustered_coordinate_blocks_v1/summary.json`
- `runs/m1544a_clustered_coordinate_blocks_v1/summary.md`

