# SAE Milestone 2 Follow-Up Experiments Report

Date: 2026-05-12

Base checkpoint:

```text
sae-milestone-1
```

## Purpose

This pass pushes the five open milestone-2 directions after the first
deep-fusion matrix:

1. selective mixed atoms with parent suppression;
2. Route 1 source-blind atoms with impact-head candidate generation;
3. atom-local calibration;
4. query-conditioned atom selection;
5. retrieval-aware SAE atom training.

Runner:

```text
scripts/research_sae_milestone2_followup_experiments.py
```

Output:

```text
results/sae/milestone2/followup-experiments/
```

Command:

```bash
python3 scripts/research_sae_milestone2_followup_experiments.py \
  --output-dir results/sae/milestone2/followup-experiments
```

A second sanity run tested lower training fanout penalty:

```bash
python3 scripts/research_sae_milestone2_followup_experiments.py \
  --training-fanout-penalty 0 \
  --training-learning-rate 0.03 \
  --training-epochs 8 \
  --output-dir results/sae/milestone2/followup-train-lowpenalty
```

## Mean Quality Matrix

| Run | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `route1_scaled_atom` | 0.7094 | 0.7951 | 0.6790 | 0.6029 | 0.5011 |
| `selective_mixed_suppress_0.5` | 0.7029 | 0.7934 | 0.6694 | 0.5929 | 0.4922 |
| `selective_mixed_suppress_0.25` | 0.7016 | 0.7923 | 0.6720 | 0.5924 | 0.4905 |
| `impact_head_8` | 0.7073 | 0.7856 | 0.6791 | 0.6034 | 0.5003 |
| `impact_head_16` | 0.7088 | 0.7964 | 0.6790 | 0.6030 | 0.5009 |
| `impact_head_32` | 0.7094 | 0.7948 | 0.6790 | 0.6029 | 0.5011 |
| `impact_head_64` | 0.7094 | 0.7951 | 0.6790 | 0.6029 | 0.5011 |
| `atom_local_qrel_lift` | 0.7036 | 0.7957 | 0.6611 | 0.5889 | 0.4844 |
| `atom_local_df_penalty` | 0.7095 | 0.7949 | 0.6724 | 0.6039 | 0.5017 |
| `query_conditioned_selector_32` | 0.6676 | 0.7701 | 0.6467 | 0.5676 | 0.4728 |
| `query_conditioned_selector_64` | 0.6869 | 0.7923 | 0.6675 | 0.5870 | 0.4929 |
| `retrieval_aware_sae_train_32` | 0.6771 | 0.7843 | 0.6611 | 0.5822 | 0.4901 |
| `retrieval_aware_sae_train_64` | 0.6849 | 0.7874 | 0.6725 | 0.5874 | 0.4945 |

## Mean Cost Proxies

These are Python research-loop counters. They are not native-index latency
claims, but they are useful for direction selection.

| Run | Query atoms | Postings touched | Candidate docs | Mean ms |
| --- | ---: | ---: | ---: | ---: |
| `route1_scaled_atom` | 91.2740 | 17655.7580 | 1980.8960 | 3.5285 |
| `selective_mixed_suppress_0.5` | 102.8040 | 17713.4920 | 1980.8960 | 3.6714 |
| `selective_mixed_suppress_0.25` | 102.8040 | 17713.4920 | 1980.8960 | 3.2957 |
| `impact_head_8` | 91.2740 | 705.3900 | 516.8160 | 4.1547 |
| `impact_head_16` | 91.2740 | 1381.6780 | 852.8940 | 6.7166 |
| `impact_head_32` | 91.2740 | 2652.3080 | 1268.1340 | 9.7852 |
| `impact_head_64` | 91.2740 | 4875.1260 | 1645.7580 | 12.1158 |
| `atom_local_qrel_lift` | 91.2740 | 17655.7580 | 1980.8960 | 3.7052 |
| `atom_local_df_penalty` | 91.2740 | 17655.7580 | 1980.8960 | 2.9998 |
| `query_conditioned_selector_32` | 45.8640 | 11623.7080 | 1806.4120 | 1.7392 |
| `query_conditioned_selector_64` | 71.6700 | 14833.7260 | 1969.9100 | 2.3980 |
| `retrieval_aware_sae_train_32` | 45.8640 | 12338.6760 | 1876.9880 | 1.9350 |
| `retrieval_aware_sae_train_64` | 71.6700 | 15213.7920 | 1975.4220 | 2.4936 |

## Direction 1: Selective Mixed Atoms

### Result

Selective mixed atoms with parent suppression did not improve the five-dataset
frontier:

```text
route1_scaled_atom Recall@100 = 0.7951, MRR@20 = 0.6790
mixed suppress 0.5 Recall@100 = 0.7934, MRR@20 = 0.6694
mixed suppress 0.25 Recall@100 = 0.7923, MRR@20 = 0.6720
```

### Interpretation

The current mixed atom generator is not selective enough. It adds atoms and
slightly changes ranking, but it does not reduce candidates:

```text
route1 candidate docs = 1980.9
mixed candidate docs  = 1980.9
```

This does not reject mixed atoms. It rejects this simple co-activation version.
The next useful mixed-atom test must enforce a harder condition:

```text
mixed atom must have lower DF than both parent atoms
+ mixed atom must have positive held-out lift
+ parent SAE suppression must reduce broad latent fanout
```

## Direction 2: Impact-Head Candidate Generation

### Result

This is the strongest systems result in the pass.

`impact_head_16` slightly beats Route 1 quality while reducing touched postings
by about `92%`:

```text
Route 1 Recall@100 = 0.7951
Head16 Recall@100  = 0.7964

Route 1 postings touched = 17655.8
Head16 postings touched  = 1381.7

Route 1 candidate docs = 1980.9
Head16 candidate docs  = 852.9
```

`impact_head_8` is more aggressive:

```text
Recall@100 = 0.7856
postings touched = 705.4
candidate docs = 516.8
```

### Interpretation

Route 1 already solved the source-blind scoring question. Impact-head
candidate generation is the first result that also materially improves the
systems story.

The likely native shape is:

```text
read query atoms
-> collect top impact postings per atom
-> union candidate docs
-> exact rerank using full source-blind atom vector
```

This is approximate candidate generation plus exact candidate rerank, not exact
full-index WAND. For RAG candidate generation, this may be the right tradeoff.

## Direction 3: Atom-Local Calibration

### Result

Unsupervised DF penalty is close to Route 1:

```text
atom_local_df_penalty Recall@100 = 0.7949
atom_local_df_penalty MRR@20 = 0.6724
```

Qrel-lift calibration improves Recall@100 slightly but hurts ranking:

```text
atom_local_qrel_lift Recall@100 = 0.7957
atom_local_qrel_lift MRR@20 = 0.6611
```

### Interpretation

Atom-local calibration is worth keeping, but not as a standalone replacement
yet. The simple qrel-lift formula overweights atoms that help recall but hurt
first-page ranking. DF penalty is safer but does not reduce fanout because it
does not change selected atoms.

The useful next step is to combine atom-local calibration with impact-head
candidate generation, where calibration changes which postings enter the head.

## Direction 4: Query-Conditioned Atom Selector

### Result

The rule-based selector reduces query atoms and postings, but quality drops:

```text
selector32 Recall@100 = 0.7701, MRR@20 = 0.6467
selector64 Recall@100 = 0.7923, MRR@20 = 0.6675
```

### Interpretation

The direction is valid, but this rule is too weak. It removes too many useful
SAE atoms and does not reduce candidates enough on broad datasets such as
Arguana.

This should not continue as a hand-written rule. It should be folded into the
retrieval-aware training route as a learned query-conditioned selector.

## Direction 5: Retrieval-Aware SAE Atom Training

### Result

The first implementation trains fold-local SAE atom reweights from qrel
positive-vs-negative pairs, then applies a query budget.

Default run:

```text
train32 Recall@100 = 0.7843, MRR@20 = 0.6611
train64 Recall@100 = 0.7874, MRR@20 = 0.6725
```

Low fanout-penalty sanity run:

```text
train32 Recall@100 = 0.7842, MRR@20 = 0.6646
train64 Recall@100 = 0.7925, MRR@20 = 0.6746
```

### Interpretation

This result is mixed but important.

The positive signal:

- SciFact `train32` improves Recall@100 to `0.9900`.
- FiQA `train32` improves Recall@100 to `0.9181` while cutting postings.
- Low-penalty `train64` nearly recovers Route 1 quality.

The negative signal:

- The global SAE atom reweighting is not robust across all datasets.
- Arguana loses Recall@100 under query budget.
- Ranking does not beat Route 1 on the five-dataset mean.

The conclusion is not "stop retrieval-aware training". The conclusion is:

```text
global SAE atom reweighting is too weak;
the next training route must be query-conditioned.
```

The next model should train a query-side selector or adapter:

```text
input:
  query SAE activation
  atom DF / fanout cost
  token-latent graph edge features
  lexical-anchor presence

output:
  selected query atoms and weights

loss:
  qrel or dense-teacher listwise ranking
  candidate-budget penalty
  lexical-anchor preservation
```

This is the most interesting model-side path, but it needs a real
query-conditioned objective rather than global weights.

## Decision

The new priority order is:

1. **Route 1 + impact-head candidate generation** is the next native-shaped
   systems route.
2. **Atom-local calibration inside impact-head selection** is the next scoring
   route.
3. **Query-conditioned retrieval-aware SAE atom selector** is the next model
   route.
4. Selective mixed atoms should continue only if the next version proves lower
   DF and held-out lift.
5. Hand-written query-conditioned selector rules should be archived.

## Next Concrete Step

Build a Route 1 impact-head v2 experiment:

```text
atom-local calibrated impact heads
+ head size sweep
+ exact candidate rerank
+ per-dataset failure analysis
```

In parallel, define the training target for a query-conditioned SAE atom
selector. That should replace the current global atom reweighting proxy.
