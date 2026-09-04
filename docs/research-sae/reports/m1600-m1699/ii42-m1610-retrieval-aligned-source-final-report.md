# M1610 Retrieval-Aligned Posting Source Final Report

## Executive Decision

**M1610 is closed as a no-go.** Neither retrieval-aligned hierarchy nor
contextual token routing produced a deterministic, qrels-free query policy
that approaches dense equivalence in one scalar unified posting index.

M1620 subsequently corrected an underfill normalization bug in the M1610B
overlap helper. The locked 8,192-key direct route is strictly O@100 `0.236960`
and O@256 `0.130109` at `0.15x`; its conditional oracle is O@100 `0.999280`
and O@256 `0.877422`. This widens, rather than reverses, the no-go gap. M1610A
filled its matched budgets and is unaffected by this specific correction.

The program did produce two durable results:

1. it found and corrected a serious matched-budget evaluator confound in the
   hierarchical branch;
2. it proved that a contextual token document source can contain the dense
   tail at `0.15x` reads, while direct query routing cannot identify the
   required keys.

This is not evidence that inverted retrieval or P1 is useless. It is evidence
against the narrower hypothesis that a pure single-pass local router can
recover dense neighbourhoods merely by changing the discrete source.

## Scientific Question

M1600 had shown a strong source oracle but a weak static query router. M1610
asked whether literature-backed source constructions could make the useful
query/document collision locally observable:

- M1610A: EHI-style conditional hierarchical paths;
- M1610B: CITADEL-style contextual token routing;
- one CoRGII-style static corpus co-occurrence repair after measured failure.

Both branches preserved the strict product boundary: one encoder surface, one
posting namespace, no ANN side index, no BM25 side index, no qrels-time gate,
and no dataset-specific policy.

## Result Matrix

| Branch/surface | O@10 | O@100 | O@256 | Reads | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| M1610A init, matched 0.15x | 0.959200 | 0.839040 | 0.729539 | 0.149844x | reference |
| M1610A trained, matched 0.15x | 0.911000 | 0.747000 | 0.634453 | 0.149844x | stop |
| M1610A source oracle | 1.000000 | 1.000000 | 1.000000 | 0.106826x | oracle only |
| M1610B 2,048 direct | 0.854400 | 0.503662 | 0.481304 | 0.050396x | capacity fail |
| M1610B 8,192 direct, strict K | 0.762800 | 0.236960 | 0.130109 | 0.026206x | observability fail |
| M1610B 8,192 qTopK=64 | 0.954400 | 0.597146 | 0.466802 | 0.149337x | tail fail |
| M1610B 8,192 co-occurrence=32 | 0.898800 | 0.474691 | 0.405490 | 0.139008x | tail fail |
| M1610B conditional oracle, strict K | 1.000000 | 0.999280 | 0.877422 | 0.149435x | oracle only |
| M1565 route2048 frontier | - | 0.906235 | 0.842683 | 0.045015x | unbeaten |

The M1610A and M1610B canaries use different heldout sizes, so their raw rows
are not a branch-to-branch ranking. Each branch is compared only against its
own initialization, oracle, fixed budget, and predeclared gate. Neither
reaches the existing M1565 quality-cost frontier.

## What The Experiments Resolve

### 1. Source capacity is not deployability

Both branches have strong oracles. M1610B's 8,192-key source reaches
O@100=`1.0` and O@256=`0.900384` at `0.149307x`. However, the oracle selects
about 167 keys per query using knowledge of the dense target. The direct
encoder emits 4.55 keys and reaches only O@100=`0.510025` and
O@256=`0.506198`.

The existence of a sparse cover therefore does not show that text-local
features can select it.

### 2. The hierarchy gain was not a training breakthrough

At a fixed 256-leaf beam, training appeared to improve O@100 by `0.06722` and
O@256 by `0.103779`. The trained source simply consumed more than twice the
postings. When both checkpoints filled the same cap, training reduced O@100
by `0.09204` and O@256 by `0.095086` at `0.15x`.

This closes the EHI-style branch on its actual hard-access surface and adds a
mandatory matched-budget rule to future work.

### 3. The token branch fails specifically on the dense tail

Increasing query Top-K from 2 to 64 raises O@10 from `0.772854` to `0.954400`,
but lowers O@256 from `0.506198` to `0.466802`. Corpus co-occurrence shows the
same inversion. The missing action is not simply "probe more related keys";
the locally most related keys are biased toward dense-head membership and do
not identify the diverse tail.

### 4. More depth is not the supported next action

The 2,048-key trained router improved DF but reduced hard overlap. The 8,192
source then failed before training because its direct policy could not expose
its oracle capacity. More steps, a larger MLP, an impact head, or another
loss-weight sweep would act after or around a missing candidate source. They
do not address the observed causal bottleneck.

## Literature Interpretation

- EHI (`arXiv:2310.08891`) supports joint path learning, but its useful result
  does not remove the need for matched access cost. M1610A's apparent gain
  disappears under that control.
- CITADEL (`arXiv:2211.10411`) validates contextual routing as a multi-vector
  efficiency mechanism. Its residual token-vector interactions are not
  equivalent to scalar posting admission, and M1610B shows that routing keys
  alone do not preserve the dense tail.
- ASI++ (`arXiv:2405.14280`) supports joint balance and matching, but balance
  is not sufficient when query-side useful-key ordering is unobservable.
- CoRGII (`arXiv:2510.22479`) motivates discrete tokenization and
  co-occurrence multiprobing. The locked M1610B audit tests that finite repair
  and finds that it improves head overlap while worsening tail overlap.

The literature mechanisms are therefore informative but do not overturn the
local product gate.

## Product And Research Verdict

Do not attach M1610 to the text encoder, run BEIR/shared15, or hide the failed
first-stage gate with BM25/reranking. Retain M549U/P1 as the frozen product
milestone and retain M1565 as the strongest bounded deterministic source
frontier.

No additional pure single-pass M1610 variant is authorized. A future program
must change at least one causal premise:

1. permit an index-internal adaptive second probe whose next posting actions
   depend on first-probe corpus evidence, while still using one unified index;
2. learn identifiers from substantially broader corpus supervision and show
   before training that target/harm keys are observable from query-time
   features; or
3. relax scalar postings to retain residual token interaction, explicitly
   accepting that this is a different product surface.

Option 1 is the only remaining direction that directly targets the measured
observability gap without introducing ANN or BM25. It must begin with a finite
oracle-to-observable audit, not another end-to-end training run. If the second
probe's useful actions are still inseparable without dense target knowledge,
the strict pure-posting replacement program should remain stopped.

## Reproducibility And Closure

- Branch baseline: `research/m1560-dense-neighborhood-posting` at `c701d20e`.
- Host: `spark-1`; `spark-2` was not disturbed.
- Container: `nvcr.io/nvidia/pytorch:26.03-py3`.
- Run root:
  `/home/huoju/leask/runs/ii42-m1610-retrieval-source-v1`.
- All long and diagnostic runs are persisted under `runs/` with JSON and
  Markdown summaries; ClearML task IDs are listed in the branch reports.
- No M1610 tmux, Docker, or Python process remains active.
- No qrels or BEIR row selected a checkpoint, source, query budget, or
  co-occurrence width.
