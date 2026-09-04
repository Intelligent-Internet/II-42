# M562 DREAM Attention-Head Probe Plan

M562 is the route-decision step after M559-M561 failed to improve on the M555
weak-positive baseline.  It stops adding likelihood-blend or weighting proxies
and checks the actual DREAM prerequisite: whether the frozen judge has
query-focused retrieval heads under our prompt.

## Why This Exists

The useful part of DREAM is not simply frozen-LM likelihood.  The paper's
training signal works because retriever scores enter selected attention heads of
a frozen LLM.  The next-token loss then sends gradients through a document-level
attention budget.

Our prior probes did not test that interface:

- M557 used student-softmax expected utility outside the LM.
- M561 used target-passage likelihood to build a teacher distribution outside
  the LM.
- Both were useful negatives, but neither proves whether true attention
  injection would work.

M562 therefore asks a smaller question first: does the frozen LM already contain
heads whose query-token attention prefers the dense-top candidate document?

## Implementation

Code:

```text
scripts/research_sae_m562_attention_head_probe.py
scripts/run_m562_attention_head_probe_spark.sh
```

The probe is qrels-free.  For each task/query:

1. Use the current dense rows to choose `teacher_pool_k` candidates.
2. Treat dense top-1 as the pseudo-positive target document.
3. Add random negatives to create a candidate set.
4. Build a document-first prompt:

```text
Document 1: ...
Document 2: ...
...
Query: ...
Target: dense-top document text
```

5. Run the frozen LM with `output_attentions=True`.
6. For every layer/head, measure attention mass from query-token rows to each
   candidate document span.
7. Subtract the same measurement under a content-free baseline query, currently
   `N/A`.
8. Rank candidates by adjusted query-to-document attention.

The output reports per-head single-positive `NDCG@10`, MRR, top-1 rate, and
mean rank for the dense-top pseudo-positive.

## Continue Gate

Continue to true differentiable score injection only if this probe finds a
stable head signal:

| Condition | Bar |
| --- | ---: |
| global task hits | `>= 2` |
| mean head `NDCG@10` | `>= 0.40` |
| mean head MRR | `>= 0.25` |

If this gate fails with `distilgpt2`, do not run M562 training on that judge.
Either try one stronger judge with the same probe, or stop DREAM-attention work
and keep M555 as the current encoder/posting route baseline.

## First Run

First probe:

```text
Host: spark-1
LM_MODEL=distilgpt2
TASKS=FiQA2018,SCIDOCS,TRECCOVID
TEACHER_POOL_K=8
RANDOM_NEGATIVES=8
MAX_PROBE_GROUPS=24
```

The probe is intentionally small.  Its job is not to produce retrieval metrics;
its job is to decide whether a real attention-injection canary is scientifically
worth implementing.

Remote output:

```text
/home/huoju/leask/runs/ii42-m562-attention-head-probe-v1/distilgpt2_probe_seed562/
```

Local copy:

```text
runs/m562_distilgpt2_probe_seed562/
```

## Result

The probe completed on CUDA in `42.449s`, with `24` examples per task.

Global top heads:

| Layer | Head | NDCG@10 | MRR | Top1 | Mean Rank | Task Hits |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 1 | 0.724421 | 0.659882 | 0.527778 | 2.111111 | 3 |
| 4 | 0 | 0.692669 | 0.639818 | 0.527778 | 2.777778 | 3 |
| 3 | 7 | 0.659982 | 0.580859 | 0.416667 | 2.513889 | 3 |
| 3 | 1 | 0.658096 | 0.590533 | 0.458333 | 2.625000 | 3 |
| 4 | 2 | 0.656352 | 0.586472 | 0.444444 | 2.750000 | 3 |

Best per-task heads:

| Task | Layer | Head | NDCG@10 | MRR | Top1 | Mean Rank |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| FiQA2018 | 4 | 0 | 0.710698 | 0.650174 | 0.500000 | 2.166667 |
| SCIDOCS | 4 | 1 | 0.871611 | 0.828869 | 0.708333 | 0.583333 |
| TRECCOVID | 3 | 10 | 0.637351 | 0.558533 | 0.416667 | 3.000000 |

## Decision

M562 passes the continue gate.

The strongest global head, `layer=4/head=1`, clears all bars:

| Gate | Bar | Observed |
| --- | ---: | ---: |
| task hits | >= 2 | 3 |
| mean NDCG@10 | >= 0.40 | 0.724421 |
| mean MRR | >= 0.25 | 0.659882 |

This is the first post-M561 positive signal that is specific to DREAM's actual
interface.  It does not prove that score injection will improve retrieval, but
it proves that the selected frozen judge/prompt has retrieval-like attention
heads worth using.

Next step: implement M563 as a small differentiable score-injection canary that
injects posting-score candidate weights into the selected heads
`(4,1), (4,0), (3,7), (3,1), (4,2)` and updates only the posting encoder, not
the LM.
