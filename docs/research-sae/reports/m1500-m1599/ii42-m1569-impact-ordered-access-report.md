# M1569 Exact Impact-Ordered Access Report

## Decision

**Stop the exact-term document-admission source family.**

The impact-ordered implementation is exact on all 1,620 query/source pairs,
but it cannot avoid decoding almost every selected posting. Mean read
reduction is only `0.07%` to `0.17%`, and every scope remains far above the
`0.30x` combined read budget.

Do not increase the document-term budget, train a selector, search a DF
threshold, or replace the exact stopping rule with approximate pruning. The
M1568 admission signal is real, but exact lexical term identities cannot
express it with both dense coverage and bounded access on this surface.

## Surface And Integrity

- Official FiQA: 57,638 documents and 648 queries.
- Frozen M1568 transductive source plus three fixed held-out sources.
- Fixed term15 document source, query term32, route1000, and lexical top256.
- Exact non-random-access impact traversal with a fixed 256-posting check
  stride.
- Lexical top256 maximum symmetric difference: `0` in every scope.
- Combined candidate maximum symmetric difference versus M1568: `0` in every
  scope.
- Qrels were never loaded.
- Read-surface SHA-256:
  `71e6b8cffdf99f262e9bb269909172710e55029eb165f042f798624a746a5edd`.
- ClearML task: `0f763865bd3749df911656fb8e487962`.
- Runtime: 219.0 seconds.

## Result

| Scope | Scan reads | Exact impact reads | Reduction | Early-stop queries | Lexical delta | Surface delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| transductive | 0.656657x | 0.655511x | 0.1745% | 2.0062% | 0 | 0 |
| heldout 1568 | 0.471672x | 0.471019x | 0.1383% | 1.2346% | 0 | 0 |
| heldout 2568 | 0.448616x | 0.448292x | 0.0722% | 0.6173% | 0 | 0 |
| heldout 3568 | 0.469185x | 0.468488x | 0.1484% | 0.9259% | 0 | 0 |

The held-out p95 combined read ratios remain `0.908x` to `0.942x`; the
transductive p95 is `1.341x`. Exact execution therefore fails not only at the
mean but also on the access tail.

## Interpretation

M1568's selected common-term lists contain valuable dense neighbors, but their
BM25 impacts are too flat to establish a useful top256 threshold before most
postings are decoded. This is not a candidate materialization artifact: an
exact impact-sorted index has the same problem.

The combined M1566-M1569 evidence closes the exact lexical compression family:

- all exact term occurrences preserve dense capacity but cost `2.55x` reads;
- unsupervised term15 is cheap but loses dense membership;
- whole-term DF filtering is cheap but loses still more membership;
- dense-teacher term15 restores held-out membership, but concentrates postings
  on high-DF terms and costs about `0.46x` reads;
- exact impact ordering cannot prune those lists.

The next source must therefore create posting keys whose document frequency is
bounded by construction while retaining dense-neighborhood identity. It
cannot be another scoring policy over the same exact-term lists.

## Next Structural Question

Before training, audit a dense-derived latent posting source with explicit
entropy and DF constraints. Candidate families must satisfy all of these by
construction:

1. one posting namespace and one native inverted index;
2. no ANN or external BM25 path at inference;
3. low-DF keys, not high-DF exact terms with a later filter;
4. qrels-free dense-neighborhood source oracle before any learned encoder;
5. exact route/candidate/read accounting on held-out queries.

The next contract must compare this source against the M1565 route frontier
and M1568 held-out oracle under the same fixed candidate and read budgets. If
no latent source passes the capacity gate, stop the single-hop candidate
generation branch and retain a protected-tail controller inside the unified
index as the product boundary.
