# M1800-M1814 Token-Routed Unified Index Final Report

Date: 2026-07-12

## Decision

The route produced a real structural result, but not a product-ready model.

- **Architecture result:** learned routing keys plus compact vector payloads can reproduce a routed token model exactly inside one physical inverted index. This is materially different from the failed scalar-posting families.
- **Model result:** the external CITADEL checkpoint is much weaker than the current PPLX dense root on BEIR quality and is research-only. It must not replace the product root.
- **Scale result:** exact learned-key traversal passes the index-size gate but fails the FiQA query-traversal gate because a few learned keys have very high document frequency.

The correct next training target is therefore a PPLX/dense-root learned router and vector-payload head with corpus-level load balancing. Static routing, query-only adapters, more payload quantization, and post-hoc threshold searches are closed.

## Product Boundary

The evaluated structure is one native inverted index:

```text
text encoder
    -> token payload vectors
    -> learned route keys and route weights
    -> one posting namespace
    -> posting entries: (document, route weight, vector payload)
    -> query opens learned keys and scores decoded payloads
```

The optional CITADEL CLS/dense branch was excluded. No VectorChord or external BM25 index is used in the mechanism proof.

This differs from the M1600-M1710 scalar-posting family in one essential way: a route key only performs candidate addressing, while the compact vector payload retains local semantic geometry for scoring. The key is not required to carry all semantic information as one scalar impact.

## Evidence Ladder

| Stage | Question | Main evidence | Decision |
| --- | --- | --- | --- |
| M1800 | Can BGE token payloads work after static spherical routing? | Candidate O@100 `0.2914`; direct O@100 `0.2854`; score retention was high only inside a poor candidate set | Static router fails |
| M1801 | Can static multiprobe recover admission? | Candidate O@100 `0.7579`, but direct O@100 `0.4959`, `1.17 MiB/query`, and `7.40x` document-equivalent reads | Multiprobe is not a product route |
| M1802 | Is the missing route locally observable? | Witness recall R@4 `0.7537`, R@16 `0.9273`, R@64 `0.9876`; oracle positive MRR `0.8851` | A learned router is justified |
| M1803 | Can a direct query-logit adapter learn the witness? | Training loss fell, held-out routing collapsed; selected checkpoint was step 0 | Stop direct adapter |
| M1804 | Can a geometry-constrained hidden adapter generalize? | No held-out improvement; selected checkpoint was step 0 | Close raw-BGE query-only router family |
| M1810 | Does a retrieval-trained token model fix static routing? | ColBERT candidate O@100 `0.9059`, but direct O@100 `0.5599`; MRR fell from `0.7612` to `0.6098` | Static route still fails |
| M1811 | Does jointly learned routing change the result? | CITADEL learned route candidate O@100 `0.9950`, direct O@100 `0.9933`; MRR `0.7974` vs exact `0.8000` | Authorize full-corpus mechanism test |
| M1813 | Is score loss caused by read caps or payload quantization? | Full selected-key traversal restored exact quality on SciFact/NFCorpus; fp32-safe quantization removed a false zero-vector failure | Payload scoring is not the blocker |
| M1814C | Can official pruning plus fp16 close the small-corpus mechanism? | Conditional O@10/O@100/O@256 all `1.0`; qrels metrics exactly match exhaustive learned routing | Architecture passes on small corpora |
| M1814D | Does the exact mechanism scale to FiQA under the fixed cost gate? | Conditional overlaps all `1.0`, but mean query payload is `1.89 MiB` and p95 is `4.66 MiB` | Scale gate fails |

The M1811 result must be judged against exhaustive **learned-route** scoring, not unrestricted token MaxSim. Routing is part of CITADEL's representation and score definition. The earlier unrestricted-MaxSim gate in the raw M1811 summary is therefore not the product gate; M1811B corrected that comparison surface.

## Full-Corpus Quality

The table compares the current PPLX dense baseline with the external CITADEL learned-route score after the official `query_topk=1`, `document_topk=5`, `route_weight>0.9` pruning used by M1814.

| Dataset | System | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| SciFact | PPLX dense | 0.747891 | 0.713623 | 0.963333 | 0.722311 | 0.993333 |
| SciFact | CITADEL learned route | 0.597058 | 0.555425 | 0.857000 | 0.564995 | 0.897667 |
| NFCorpus | PPLX dense | 0.360615 | 0.168011 | 0.326964 | 0.578555 | 0.421287 |
| NFCorpus | CITADEL learned route | 0.305252 | 0.136880 | 0.243518 | 0.512135 | 0.298747 |
| FiQA | PPLX dense | 0.516778 | 0.457649 | 0.828868 | 0.601742 | 0.896843 |
| FiQA | CITADEL learned route | 0.245051 | 0.196721 | 0.509054 | 0.310382 | 0.607312 |

The architecture proof is strong because M1814's native posting traversal exactly matches its exhaustive learned-route control. The checkpoint quality is not competitive: all three datasets are below the PPLX dense root, with the largest gap on FiQA. This is a root/training problem, not an inverted-index scoring discrepancy.

## Native Cost

Fixed gates:

- mean decoded query payload at most `1 MiB/query`;
- index payload at most `10 KiB/document`;
- conditional O@10/O@100/O@256 at least `0.999` against exhaustive learned routing;
- qrels metrics within `0.001` of exhaustive learned routing.

| Dataset | O@100 | KiB/query mean | KiB/query p95 | KiB/doc | Postings/doc | Max list DF | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| SciFact | 1.000000 | 346.52 | 756.27 | 5.70 | 81.04 | 4,029 | Pass |
| NFCorpus | 1.000000 | 62.77 | 229.55 | 5.49 | 78.08 | 4,187 | Pass |
| FiQA | 1.000000 | 1,936.32 | 4,775.59 | 3.53 | 50.13 | 44,142 | **Fail query cost** |

FiQA is the decisive diagnostic. Pruning reduces storage, and fp16 preserves the learned-route ranking exactly, but selected query keys can still open lists containing most of the corpus. The failure is high-DF routing-key load, not payload width, quantization error, read caps, or an overlap accounting bug.

## What Is Established

1. A one-index semantic posting system does not need to collapse semantics into a non-negative scalar per vocabulary term. Learned keys can address compact vector-bearing postings, and the payload scorer can preserve the model's own ranking exactly.
2. Static spherical routing is insufficient even with stronger token representations and large multiprobe budgets. Routing must be learned jointly with the token payload space.
3. Query-only router adapters do not generalize from local witness supervision. The router and payload geometry must be trained together.
4. The external learned router proves representation capacity but not product quality. It was trained for a different root and retrieval surface.
5. Corpus-global key load is now the primary engineering/training constraint. Local candidate or listwise losses cannot observe this constraint by themselves.

## Next Valid Experiment

The only justified continuation is a new PPLX/dense-root CITADEL-like model, not another M1600-style scalar output head.

1. Freeze the current dense root and expose token-level states without changing the dense control score.
2. Jointly train a learned route head and compact signed vector-payload head to reproduce the dense root's candidate and ranking geometry.
3. Include a corpus-global route-load objective that directly penalizes high DF and skewed key utilization. Batch-local FLOPS or average sparsity is not an adequate substitute.
4. Keep payload compression and retrieval quality as separate gates. First prove exact/near-exact scoring on admitted keys, then prove key admission and load.
5. Validate in order: paired canary, SciFact/NFCorpus native closure, then FiQA. Do not expand to BEIR15 if FiQA remains above `1 MiB/query` or if dense quality floors fail.

Promotion requires both:

- at least `0.99` overlap against the PPLX dense root at O@100 with no material qrels regression;
- FiQA mean query payload at most `1 MiB` and index payload at most `10 KiB/document`.

If a jointly trained router cannot satisfy the FiQA load gate without losing dense quality, close the one-index vector-payload route. That result would establish that the remaining limit is the posting access pattern, not insufficient loss exploration.

## M1820 Follow-Up

M1820 executed the proposed frozen-PPLX joint-router experiment with a canonical pooled-root summary token, contextual tokens, a jointly trained 4,096-key router, and a 32-dimensional signed payload head.

The fixed 512/128-query S0 gate failed. Best direct dense O@100 improved from `0.375469` to `0.405156`, but the `+0.029688` gain missed the required `+0.05`. Candidate-upper O@100 remained near `0.60` and ended at `0.595156`, far below the `0.80` signal floor. Step 400 did not recover the gap.

Therefore the PPLX head-only continuation is closed before load training or native BEIR evaluation. The external CITADEL mechanism remains a representation proof, but reproducing it from the PPLX root would require a new large-scale end-to-end retriever-training program rather than another compiler/head iteration.

## Reproducibility And Licensing

- M1803 ClearML task: `7af82e8fdcf34e848db53d1ec993859a`.
- M1804 ClearML task: `d53e6c7a7def4697937e72cfad1e443f`.
- External CITADEL checkpoint SHA-256: `23b7e4f7e355d0d5d26c885b55b04974b831364f7aa4098d5c0760e42c94e268`.
- The external CITADEL implementation/checkpoint is CC-BY-NC and is used only as a research mechanism oracle. A product implementation and checkpoint must be independently trained under an appropriate license.
- Primary adjacent work: [CITADEL](https://arxiv.org/abs/2211.10411), [COIL](https://arxiv.org/abs/2104.07186), [ColBERTv2](https://arxiv.org/abs/2112.01488), and [PLAID](https://arxiv.org/abs/2205.09707).

## Final Recommendation

Keep learned-key/vector-payload retrieval as a validated adjacent architecture, but stop the current product route. M1820 has now tested and rejected the carefully gated PPLX-root head-only continuation. No further router, threshold, quantization, payload-width, or local-loss variant is warranted under the present one-index compiler program.
