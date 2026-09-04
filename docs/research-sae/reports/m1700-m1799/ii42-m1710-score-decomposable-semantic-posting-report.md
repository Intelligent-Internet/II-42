# M1710 Score-Decomposable Semantic Posting Report

## Decision

**Stop M1710 at the qrels-free heldout representation-capacity gate. Do not
run official FiQA, build a native index, or train an encoder from this source.**

M1710 tested the one score mechanism missing from M1600: balanced codeword
postings directly accumulate an approximate dense inner product instead of
forming a binary candidate union followed by exact-dense reranking. A second
deterministic residual codebook increased reconstruction quality, but even its
unbounded all-code ceiling reached only O@100 `0.524210` and O@256 `0.559961`.
The required ceiling is `0.98/0.95`.

This is not an evaluator failure. The exact rotated-dense control reproduced
cached dense rankings at O@10 `1.000000`, O@100 `0.999590`, and O@256
`0.999688`.

## Research Question

M1520-M1701 repeatedly exposed a quality/cost conflict for one vocabulary-
sparse output. M1710 moved the question before encoder training:

> Does a deterministic balanced or residual code representation itself have
> enough capacity to preserve dense top-k through one additive inverted score?

The audit used no qrels, BM25, cross encoder, dataset identity, or model
training. It reused the frozen M1600 qrels-free dense pool and initial 8 x 512
codebook.

## Literature Interpretation

The tested representation follows the decomposable inner-product structure
behind product and residual quantization. The result is consistent with, but
narrower than, the published literature:

- [Distill-VQ](https://arxiv.org/abs/2204.00185) and
  [RepCONC](https://arxiv.org/abs/2110.05789) train quantization with retrieval
  and balance objectives because reconstruction or nearest-code assignment is
  not sufficient.
- [Anisotropic Vector Quantization](https://arxiv.org/abs/1908.10396) shows
  that MIPS needs a direction-sensitive error objective rather than ordinary
  isotropic reconstruction.
- [Searching Dense Representations with Inverted Indexes](https://arxiv.org/abs/2312.01556)
  independently finds that dense features can be represented in an inverted
  index, but useful quality requires impractical traversal.
- [ColBERTv2](https://arxiv.org/abs/2112.01488) and
  [PLAID](https://arxiv.org/abs/2205.09707) do not collapse retrieval into one
  sparse dot product. They preserve token-level multi-vectors, centroid
  pruning, residual compression, and later interaction. Their success is
  evidence for retaining structure, not evidence that a frozen single-vector
  compiler is lossless.

## Frozen Surface

| Field | Value |
| --- | ---: |
| Dense root | `BAAI/bge-base-en-v1.5` |
| Codebook train documents | 88,992 |
| Heldout documents | 8,988 |
| Heldout queries | 1,000 |
| First-level namespace | 8 x 512 keys |
| Residual namespace | 8 x 256 keys |
| Residual fit sample | 20,000 documents |
| Residual K-means steps | 10 |
| Qrels usage | none |

Input SHA-256 values:

- train cache: `4c0d8a57bf740b1b1641a22230d8787eaf251674ee5fc0918f39b623992bb6ff`;
- heldout cache: `981c6e1ec18602b7efc10b3a4765ead045c4ecbf576b9f1749bb3f044fbd55b4`;
- frozen codebook: `b3adfaf691a5a5ae68920c7065f731b663dadda33fc08889a976c3f8729ec944`.

The residual fit was healthy. Every group had zero empty clusters. Final
per-group residual MSE ranged from `0.044746` to `0.046129`, and centroid
movement fell to roughly `2e-5` after ten steps.

## Representation Ceiling

The ceiling scores every document with every code. It ignores traversal cost
and therefore answers representation capacity before access policy.

| Representation | O@10 | O@100 | O@256 | Score Pearson | Reconstruction cosine |
| --- | ---: | ---: | ---: | ---: | ---: |
| Exact rotated dense control | 1.000000 | 0.999590 | 0.999688 | 1.000000 | 1.000000 |
| First-order 8 x 512 code | 0.316300 | 0.444940 | 0.491781 | 0.759800 | 0.746489 |
| First + residual code | 0.497600 | 0.524210 | 0.559961 | 0.820708 | 0.779609 |

Residual quantization improves every diagnostic substantially, but the gap is
too large to attribute to a query probe policy. The representation fails before
the cost gate is applied.

## Bounded Posting Frontier

`Upper` reranks the exact same touched-document set with exact dense scores. It
is diagnostic only and cannot be deployed.

| Policy | Direct O@100 | Direct O@256 | Upper O@100 | Upper O@256 | Reads | Mean touched |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| first 4/group | 0.429040 | 0.417516 | 0.704780 | 0.546809 | 0.076215x | 0.050928 |
| first 8/group | 0.448530 | 0.455977 | 0.830990 | 0.704930 | 0.146092x | 0.092631 |
| first 16/group | 0.457770 | 0.482340 | 0.917890 | 0.841230 | 0.282470x | 0.166641 |
| residual 2+2/group | 0.390935 | 0.396611 | 0.566425 | 0.417525 | 0.041964x | 0.029611 |
| residual 4+4/group | 0.430450 | 0.417790 | 0.718190 | 0.560087 | 0.084341x | 0.057943 |
| residual 8+8/group | 0.451570 | 0.459727 | 0.857410 | 0.740117 | 0.183656x | 0.123333 |

Every row meets its configured read budget. First-level maximum DF is
`0.009457`; combined first/residual maximum DF is `0.012461`, below the `0.02`
gate. More balanced keys and lower DF therefore do not solve either dense
admission or additive score fidelity.

## Causal Diagnosis

M1710 separates the failure into two independent parts:

1. **Representation failure.** Even an all-key scan cannot make the residual
   reconstruction preserve dense top-k. More query probes cannot fix this.
2. **Bounded access failure.** At the largest allowed first-order policy, exact
   dense reranking reaches only O@100 `0.917890` and O@256 `0.841230`.
   Useful documents are absent before the approximate posting score is used.
3. **Score failure.** On the same touched set, direct posting scoring is much
   weaker than the exact-dense upper. A reranker would hide rather than solve
   the one-index objective.

The result closes score-decomposable reuse of the frozen M1600 codebook. It
does not claim a mathematical impossibility theorem for every conceivable
inverted representation.

## Structural Evidence Across M1520-M1710

| Family | Capacity result | Cost/access result | Verdict |
| --- | --- | --- | --- |
| M1520C token-state codebook | weak dense-miss recovery | touch acceptable | source misaligned |
| M1541/M1630 signed PCA | near-dense after broad accumulation | full union/all blocks | expressive, not selective |
| M1570 composite product cell | oracle almost saturates dense | deterministic cell order weak | capacity unobservable |
| M1571/M1572 cosine LSH | high radius coverage | hot buckets and weak votes | non-selective |
| M1600 balanced multicode | source oracle near perfect | nearest/router policy weak | query/source compatibility missing |
| M1640-M1701 vocabulary sparse | real native retrieval | quality gains raise DF/traversal | stable Pareto conflict |
| M1710 additive residual code | full-code O@100 0.524 | bounded upper O@100 0.918 | representation and access fail |

No tested post-hoc compiler over a frozen dense vector simultaneously preserves
dense top-k, exposes it through qrels-free query postings, and remains within a
bounded single-hop traversal.

## Final Route Verdict

M1710 rejects the proposed next step as an encoder target. Training a text head
to imitate this compiler would faithfully reproduce a representation that
already fails its oracle-capacity gate.

The evidence-backed options are now narrower:

1. **Keep the strict product boundary:** representation and index must be
   co-designed from the dense root, with retrieval-oriented quantization and
   corpus-load constraints changing the geometry itself. This is a new model,
   not another output head, and M1600 warns that ordinary joint training is
   high risk.
2. **Keep one physical index but relax single-hop additive scoring:** use
   geometrically cohesive blocks, centroid pruning, residual summaries, or
   late interaction inside the index. This follows the mechanism of PLAID or
   Seismic but is no longer the original one-vector posting-dot contract.
3. **Keep the frozen dense geometry:** accept an ANN/vector component. This is
   operationally conventional but does not satisfy the research product goal.

There is no justified M1711 loss, teacher, code-count, group-count, probe, or
seed sweep. Any continuation requires an explicit architectural decision
between geometry/index co-design and a relaxed multi-stage single-index
engine.

## Reproducibility

- Host: `spark-2`; image `nvcr.io/nvidia/pytorch:26.03-py3`.
- Run:
  `/home/huoju/leask/runs/ii42-m1710-score-decomposable-v1/runs/m1710a-score-decomposable-heldout-v2`.
- Archive:
  `/Volumes/Betty/II42/m1710/runs/m1710a-score-decomposable-heldout-v2`.
- ClearML was unavailable in the container (`No module named clearml`); the
  immutable summary, log, source, input hashes, and codebook artifact are the
  audit record.
