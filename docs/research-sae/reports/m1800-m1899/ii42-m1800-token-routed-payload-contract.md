# M1800 Token-Routed Unified Payload Contract

## Question

M1610B proved that contextual token keys can form a high-capacity document
source, but it deliberately collapsed every key to a scalar document posting.
M1727 then found additional candidate capacity, while M1728 and M1730 showed
that a scalar or fixed document-level residual cannot resolve documents inside
the opened posting lists.

M1800 tests the missing representation directly:

```text
frozen contextual token state
    -> semantic routing key
    -> key -> (document id, compact token payload)
    -> query-token local interaction inside one inverted index
```

This is one physical inverted index. It is not ANN plus BM25 and it does not
use exact pooled-dense reranking. Lexical keys may share the same index in a
later stage, but M1800A first isolates semantic capacity.

## Prior Art Boundary

COIL (`arXiv:2104.07186`) stores contextual token representations in lexical
inverted lists. CITADEL (`arXiv:2211.10411`) learns dynamic lexical routing and
retains residual token-vector interaction. SLIM (`arXiv:2302.06587`) maps
contextual tokens into sparse lexical space and refines a first-stage inverted
retrieval score.

M1800 does not claim these mechanisms as new. The project-specific question is
whether the existing II42 dense root, M1610 routing source, native posting
budgets, and single-index lifecycle can support this less restrictive scoring
algebra.

## M1800A Frozen Capacity Audit

Inputs are immutable M1610B heldout artifacts:

- frozen BGE token states;
- frozen pooled-dense vectors and dense top256;
- the locked 8,192-key contextual router initialization;
- disjoint validation weak pairs.

No qrels, dataset identity, checkpoint selection, or training is allowed.

The audit compares:

1. cached pooled-dense control;
2. exhaustive token MaxSim;
3. routed token MaxSim with full fp16 payloads;
4. deterministic projected and int8 payload controls;
5. exact pooled-dense and exhaustive-MaxSim candidate uppers over the same
   routed candidate sets.

Every row reports fixed-denominator O@10/O@100/O@256, positive top1/MRR@100,
decoded token entries, unique candidates, index bytes, payload bytes, maximum
document DF, and budget fill. Read budgets are normalized by document count so
that underfilled policies cannot appear cheaper without being visible.

## Gates

Harness integrity requires the recomputed pooled-dense ranking to match the
cached top256 exactly.

Router training is authorized only if one frozen routed policy satisfies all
of the following on heldout data:

- candidate-upper O@100 against exhaustive MaxSim is at least `0.90`;
- routed direct O@100 retains at least `80%` of that candidate upper;
- document-equivalent decoded token entries are at most `1.0x`;
- projected int8 payload O@100 retains at least `95%` of the corresponding
  full-payload routed score;
- positive MRR@100 is no worse than pooled dense by more than `0.02`.

These are representation gates, not product promotion gates. Passing M1800A
authorizes a frozen-root router/projection training canary. It does not
authorize BEIR, native default promotion, or a broader model run.

## Stop Conditions

Stop before training if:

- exhaustive token MaxSim itself has no useful heldout signal;
- routed candidate capacity misses the gate;
- direct routed scoring retains less than `80%` of candidate capacity;
- compact payloads destroy the full-payload result; or
- the only passing policy requires scanning an unbounded fraction of token
  postings.

If M1800A passes, M1800B may train only a frozen-root token router/projection.
The dense backbone remains frozen until a native heldout replay passes.
