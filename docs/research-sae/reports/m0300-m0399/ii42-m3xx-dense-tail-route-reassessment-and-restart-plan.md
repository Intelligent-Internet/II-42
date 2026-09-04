# II-42 M3xx Dense-Tail 路線完整評估與重啟計劃

日期：2026-07-15

狀態：歷史路線重評估，尚未重啟

建議代號：`M3XX-R`（M3xx structural restart）

## 1. 執行摘要

M3xx 不是一條已經成功產品化、後來被無故放棄的路線，也不是一條
完全失敗、沒有必要再看的路線。更準確的結論是：

1. `M379-M396` 成功證明了 **dense tail 可以被低失真地壓縮，並在已有
   候選集合上恢復接近 dense 的排序能力**。
2. `M392/M396` 的 sampled-face 品質數據是真實的表示容量證據，但當時把
   「最後只 rerank 5%-10% 文檔」誤讀成「倒排索引只讀 5%-10% 文檔」。
3. `M1541` 後來證明 historical M392 admission 實際上先 union 全 corpus，
   平均讀取約 `13.8481` 個 posting edge / corpus document，然後才把集合裁到
   約 8%。因此其低 touch 成本結論不成立。
4. `M1630` 又證明一般 global exact block-max 對 signed PCA posting 幾乎無法
   剪枝：32/32 blocks 全部打開。不能只把舊 M392 放進 BMP 就認為成本問題
   已解。
5. `M1565/M1566` 進一步定位了真正瓶頸：tail scorer 已有足夠品質，問題在
   **固定候選預算內的低成本 admission/source construction**。語義 route 與
   lexical membership 的組合具有容量，但 deterministic selector 與高 DF
   traversal 還沒有同時過關。

因此，M3xx 最值得保留的是：

- signed active contribution + compact dense-tail residual scorer；
- joint-PCA 256 / doc-side int8 幾乎無損的壓縮結論；
- semantic route 與 lexical rescue 在同一 physical inverted index 中互補的
  結構性證據。

應正式淘汰的是：

- signed dense coordinate 本身作為第一階段 admission posting；
- 用 corpus 百分比表達成本；
- 把 candidate cap 當作 posting traversal cost；
- 在 source capacity 未過關前訓另一個 router、residual model 或 alpha gate。

**總判斷：值得做一次有上限、有 gate 的結構性重啟；不值得重新打開無限的
M3xx loss/selector 微調。**

## 2. M3xx 到底是什麼路線

M3xx 並不是單一模型。它經歷了四個不同階段：

| 階段 | 版本 | 核心問題 | 結論 |
| --- | --- | --- | --- |
| 下游修復 | M300-M353 | atom candidate 能否靠 admission/ranker/fusion 轉成品質 | 有信號，但沒有解決表示幾何 |
| dense-faithfulness | M360-M369 | posting 是否能保存 dense neighborhood | signed coordinate 能保存品質，但 fanout 不可接受 |
| neural transcoder | M370-M378 | learned selector/reranker 能否補上 dense gap | router 不是主要 blocker，neural residual 不穩定 |
| structural tail | M379-M399 | 保留 active posting，額外壓縮 dense tail | tail scorer 成功；admission 成本後來被推翻 |

### 2.1 M300-M353：候選與 scorer 的局部優化盆地

這一段建立了幾個重要事實：

- atom/posting features 確實帶有 retrieval signal；
- candidate upper bound 經常很高；
- interaction scorer、boundary-aware scoring、anchor policy 能產生局部增益；
- 但 learned selector/ranker 無法穩定把候選 headroom 轉成 top-rank 品質；
- 很多版本在修補 scorer，而不是修復 posting representation。

這一段不是無價值，但它告訴我們不能在 representation/source 還沒有解決時，
把更多 qrel loss、LambdaRank、gate 或 alpha 當成突破。

### 2.2 M360-M369：第一次找到 dense-faithful posting

M366/M367 的 signed dense-coordinate posting 在 local BEIR15 sampled face 上
接近 dense：

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Mean touched |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense exact | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 1.0000 | 1.0000 |
| coordinate k384 | 0.8523 | 0.8685 | 0.7719 | 0.6921 | 0.8820 | 1.0000 |
| coordinate k512 | 0.8518 | 0.8719 | 0.7745 | 0.6944 | 0.9346 | 1.0000 |

但低 k 時品質不夠，高 k 時幾乎 touch 全 corpus。這證明：

> dense 可以被 posting score 表達，不代表它可以被低成本 posting admission
> 實現。

### 2.3 M377-M378：router 與 neural residual 不是答案

M377 從可觀測 query/corpus/pool features 選擇 scorer，最終幾乎退化成固定
`sparse_pca`，只有噪聲級差異。M378 的 neural residual 對 TREC-COVID 有局部
改善，但傷害 FiQA，macro 回退。

這兩個失敗的重要性在於：它們排除了「只差一個小 router/MLP」的簡單解釋。
缺失的是被 representation 丟掉的 dense 信息，而不是 scorer 名稱選錯。

### 2.4 M379-M391：真正成功的 dense-tail 分解

M379 找到正確 score decomposition：

```text
score(q, d)
  ~= dot(doc_active, query_full)
   + dot(doc_tail_sketch, query_full_sketch)
```

只算 tail-tail 不夠，因為它遺漏 active-document/full-query cross terms。修正後：

- joint-PCA 256 在 query-heldout split 接近或匹配 touched-set upper bound；
- 三個 heldout seeds 都有穩定正向結果；
- doc-side int8 幾乎無損；
- int4 也很接近，但只應作 aggressive storage option。

三個 query-heldout seeds 的 NDCG@10：

| Seed | Sparse | Sketch256 | Dense | Upper | Delta vs sparse |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 379 | 0.7690 | 0.7752 | 0.7748 | 0.7752 | +0.0062 |
| 1379 | 0.7715 | 0.7816 | 0.7812 | 0.7811 | +0.0101 |
| 2379 | 0.7536 | 0.7606 | 0.7613 | 0.7613 | +0.0070 |

這是 M3xx 最可靠、至今仍應保留的研究成果。

### 2.5 M392-M396：品質里程碑，但不是正式成本證明

M392 runtime-shaped prototype 將以下元件組合：

- signed coordinate posting admission；
- per-document joint-PCA tail int8 sketch；
- touched-set tail scoring；
- optional BM25 candidate/fusion。

local sampled BEIR15 標準矩陣：

| System | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | 報告 touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| Dense exact | 0.8545 | 0.8677 | 0.7748 | 0.6931 | 1.0000 |
| Dense+BM25 exact zblend | 0.8572 | 0.8620 | 0.7773 | 0.7004 | 1.0000 |
| M392 unified tail+BM25 | 0.8557 | 0.8648 | 0.7773 | 0.6983 | 0.1338 |

M396 在 MTEB10/三 seeds 的最佳 global alpha 為 `0.18`：

- mean NDCG@10：`0.58804`；
- seed std：`0.00099`；
- 相對同 surface dense+BM25：`+0.00529`；
- 相對同 surface dense：`+0.01541`；
- alpha 0.15-0.20 是穩定區間；
- 繼續 alpha sweep 沒有研究價值。

這些結果說明 tail score 與 lexical signal 的組合有真實品質價值。但必須加上
兩個限制：

1. BEIR15 是 local shared/sample surface，不是 official full-corpus proof；
2. `0.1338 touch` 是裁後 candidate/rerank 視角，不是 honest posting reads。

## 3. 為什麼當時看起來成功，後來卻停掉

核心原因是 **成本定義錯位**。

舊報告的 mental model 是：

```text
query postings -> 8% candidate docs -> tail rerank
```

後來 M1541 的 honest accounting 顯示實際上更接近：

```text
query postings
  -> read ~13.8481 posting edges / corpus document
  -> raw union = 100% corpus
  -> accumulate/select
  -> retain ~8% docs
  -> tail rerank
```

所以 8% 只描述最後 scorer 的工作集，不描述倒排引擎為得到該集合所做的工作。

### 3.1 M1541：把 scorer capacity 與 admission cost 分開

三個 canary 的 macro：

| Source | NDCG@10 | Recall@100 | Dense O@100 | Posting reads | Raw union | Rerank |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| exact BGE | 0.667473 | 0.767131 | 1.000000 | 1.0000x | 1.0000 | 1.0000 |
| historical tail256 | 0.667459 | 0.766407 | 0.921333 | 13.8481x | 1.0000 | 0.0802 |

這個結果非常重要：tail256 幾乎保留 exact quality；失敗的是 admission cost。

在 honest 15% posting-edge budget 下，quality 崩潰，且 exact-dense rerank 無法
修復，因為 positives 在 admission 階段已經丟失。這正式關閉了
signed-coordinate admission。

### 3.2 M1542/M1565：route structure 有改善，但固定預算仍困難

M1542 使用 dual spherical route，在同樣 15% union 下：

- macro NDCG@10：`0.650569`，約為 exact dense 的 97.5%；
- macro Recall@100：`0.723467`，約為 exact dense 的 94.3%。

這是顯著的結構性改善，但仍未過 dense-equivalence gate。

M1565 把 corpus percentage 改成固定候選預算後，定位更精確：

| Budget | Policy | O@100 | O@256 | Recall@100 | NDCG@10 | Reads |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 1,000 | deterministic | 0.808503 | - | - | - | - |
| 1,000 | oracle | 0.933225 | 0.698085 | 0.714086 | 0.394805 | 0.019772x |
| 2,048 | oracle | 0.997562 | 0.855155 | 0.733470 | 0.400541 | 0.043694x |

2,048 candidate 的 oracle 已幾乎恢復 dense top100，但 O@256 仍只有 0.855。
因此有兩個不同 blocker：

1. deterministic query route selector 不夠好；
2. 更深 dense tail 的 source coverage 仍不足。

### 3.3 M1566：semantic route + lexical membership 的互補性

FiQA 上：

| Variant | O@100 | O@256 | Recall@100 | NDCG@10 | CUB | Reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| route + full terms | 0.857392 | 0.757409 | 0.721033 | 0.400766 | 0.878106 | 2.552668x |
| route + full-term oracle | 0.992485 | 0.985448 | 0.731927 | 0.400680 | 0.905573 | 2.552668x |
| route + term15 | 0.843287 | 0.741000 | 0.710759 | 0.395886 | 0.862693 | 0.044619x |
| route + term15 oracle | 0.859645 | 0.763847 | 0.714375 | 0.397032 | 0.868109 | 0.044619x |

完整 lexical terms 使 oracle 幾乎恢復 dense top100/top256，證明 route 與 exact
lexical membership 在一個索引中互補。但高 DF terms 造成 `2.55x` corpus reads。
Term15 解決成本，卻失去容量。

這正是新的 M3xx 重啟需要解決的窄問題：

> 如何讓低 DF lexical rescue 與 semantic route 提供互補 coverage，同時避免
> high-DF posting traversal。

### 3.4 M1630：一般 BMP 不能直接救 signed coordinates

M1630 的 exact signed block engine 有完整 parity，但 source order 與 balanced
tree order 都打開 `32/32` blocks，decoded/full 為 `1.0`。

原因不是 BMP 理論錯，而是 signed PCA coordinate 在 global document order 下
缺乏 block cohesion；safe bound 幾乎總高於 top100 threshold。

後來 M1910/M1912A 證明 raw posting union 會高估某些 non-negative learned
sparse shape 的實際 BMP 成本。因此不能把「raw union 高」當作通用 stop rule。
但這不會推翻 M1630 對 M392 signed shape 的實測結論。重啟若要使用 block
pruning，必須改成 route-local、residual-geometric blocks，不能重跑同一 global
BMP。

## 4. 現狀判斷

### 4.1 已被證明的能力

- dense tail residual 在 touched candidate set 上足夠強；
- joint-PCA 256 是可靠的 near-dense score codec；
- doc-side int8 是安全的首選壓縮；
- semantic route 與 lexical membership 有互補性；
- 2k 級 candidate budget 下，source oracle 對 top100 已接近充分；
- M392/M396 的品質上界值得作為重啟 scorer baseline。

### 4.2 尚未證明的能力

- M392 沒有 official full-corpus、native latency 的產品證明；
- 沒有 qrels-free deterministic route selector 在固定 2k-4k budget 下跨 corpus
  過關；
- 沒有 route-local residual block engine 的 exact/approx native 成本數據；
- 沒有證明 2k-4k candidates 可以同時守住 O@100、O@256、Recall 與 row safety；
- 沒有證明 lexical rescue 可在低 DF/低 read cost 下保留 full-term oracle 的
  coverage；
- 沒有一個 unified encoder 能直接生成已驗證的 route + residual structure。

### 4.3 當前產品地位

M392/M396 **不是 deployable candidate**。它們應降級為：

1. dense-tail score capacity ceiling；
2. residual codec baseline；
3. 新 admission/source route 的 second-stage scorer；
4. 衡量新結構是否真正優於歷史路線的固定參考。

### 4.4 是否值得重啟

值得，但只值得一次 bounded structural restart。

| 維度 | 評估 |
| --- | --- |
| 科學價值 | 高：可以分離 source、selector、codec、engine 四類失敗 |
| 品質潛力 | 中高：tail scorer 與 route/lexical oracle 已有證據 |
| 成本突破把握 | 中：需要新的 route-local geometric pruning，而非舊 BMP |
| 嚴格 dense-equivalence | 中低：O@256 是明顯風險 |
| 直接重跑 M392 的價值 | 低：只會重現已知 sampled quality 與已知 cost failure |
| bounded restart 的價值 | 中高：Phase 1/2 可以快速給出結構性 go/no-go |

## 5. 新的唯一推薦架構

```text
text
  -> frozen dense encoder
       -> semantic route assignments
       -> compact residual/tail code
       -> low-DF lexical rescue keys
            -> one physical inverted index
                 - semantic route posting lists
                 - lexical rescue posting lists
                 - route-local residual-geometric blocks
            -> fixed 2k-4k candidate documents
            -> frozen M392 dense-tail score
            -> top100
```

這仍是一個 physical inverted index，不是 VectorChord + BM25 兩套外部引擎。
但它承認原生倒排內部需要兩個步驟：

1. route/lexical postings 負責 admission；
2. compact residual payload 負責在候選內恢復 dense geometry。

如果產品要求只能有一次 scalar posting accumulation、不能讀 doc residual
payload，那麼 M1541/M1630 已經顯示 near-lossless M392 能力大概率不可達。

## 6. 現代化優化方向

### 6.1 Retrieval-oriented semantic route，而不是 signed coordinate route

先比較以下 deterministic source，不訓 encoder：

1. nearest centroid / nearest-two control；
2. primary centroid + orthogonality-amplified secondary assignment；
3. load-balanced constrained clustering；
4. multi-code residual centroid assignment。

[SOAR](https://arxiv.org/abs/2404.00774) 的關鍵啟發是 secondary assignment
不應只是第二近 centroid；它應補償 primary quantization failure，避免兩次 assignment
產生高度相關的錯誤。[Distill-VQ](https://arxiv.org/abs/2204.00185) 與
[RepCONC](https://arxiv.org/abs/2110.05789) 支持 retrieval-oriented codebook
distillation 與 load balancing。

只有 deterministic source oracle 先過 gate，才值得訓 query route selector。

### 6.2 Route-local residual-geometric blocks

M1630 失敗的是 global document-order block。新結構應在每個 semantic route
內，按 residual geometry 聚類成 cohesive blocks，為每個 block 存 summary/bound。

[Seismic](https://arxiv.org/abs/2404.18812) 的核心價值不是直接套用其全部引擎，
而是證明 learned sparse retrieval 可以使用幾何 cohesive blocks 與 summary vectors
做 aggressive approximate pruning。可同時保留 ordinary BMP 作 control，並參考
[Block-Max Pruning](https://arxiv.org/abs/2405.01117) 與
[cluster-based sparse retrieval](https://arxiv.org/abs/2404.08896)。

### 6.3 固定候選數，不再使用 corpus percentage

所有新結果都必須報：

- candidate budget：512 / 1k / 2k / 4k / 8k；
- posting entries read；
- unique candidate docs；
- blocks opened / total blocks；
- residual bytes read；
- index bytes/doc；
- p50/p95 latency；
- query-weighted work `W(q) = sum(df(atom))`。

M1963 已表明，即使 corpus size、doc nnz、maxDF 相近，不同 query distribution
仍可產生非常不同的 posting work 與 p95。maxDF threshold 不是充分成本模型。

### 6.4 Low-DF lexical rescue，而不是完整 BM25 duplicate

lexical namespace 只承載 semantic route 不穩定覆蓋的部分：

- entity、rare term、exact phrase、identifier；
- lexical unique candidates；
- dense route miss 的 corpus-derived pseudo examples。

不應把完整 BM25 score 再複製到索引中。需要直接控制每個 lexical key 的 DF
與 query-weighted work。[DF-FLOPS](https://arxiv.org/abs/2505.15070) 支持直接
控制 document frequency，而不只控制平均 activation。

### 6.5 Residual payload 壓縮

固定 M392 scorer 後依次比較：

| Payload | 大致 payload/doc | 角色 |
| --- | ---: | --- |
| int8-256 | 256 B | conservative parity baseline |
| int4-256 | 128 B | historical aggressive control |
| PQ64B | 64 B | product candidate |
| PQ32B | 32 B | aggressive product candidate |

先測 deterministic compiler，不先訓 text-to-code encoder。若 oracle 不成立，
encoder 不可能修復表示容量。

### 6.6 可能的成本下降幅度（僅為假設）

以下是待 native 驗證的工程假設，不是現有結果：

- 128 signed coordinate postings/doc 改成 2-4 semantic route postings/doc，
  semantic edges 可能降低約 32-64 倍；
- 5M corpus 上 8% rerank 約 400k docs，改成固定 2k-5k candidates，residual
  payload reads 可能降低約 80-200 倍；
- int8-256 改成 PQ32-64B，payload 可再降低 4-8 倍。

這些幅度只有在 route source quality 過關時才有意義。

## 7. 暫定重啟計劃

### Phase 0：artifact identity 與 parity（0.5-1 天）

目標：確認重啟測的是歷史 M392，而不是相似但不同的模型/根目錄。

工作：

- 鎖定 dense checkpoint path、size、mtime/hash；
- 鎖定 joint-PCA projection、mean、dimension、seed；
- 鎖定 active coordinate 規則、signed score convention、quantization scale；
- 鎖定 shared/sample root 與 official/full root，禁止混用；
- 在舊 sampled face 重現 M390/M392 score parity；
- 產生 manifest，後續所有 index/result JSON 引用同一 manifest hash。

Gate：

- int8-256 scorer 與 historical result 在容許誤差內一致；
- source identity 完整；
- 若 identity 無法恢復，只能把舊結果作歷史參考，不能宣稱重現。

### Phase 1：deterministic source capacity（FiQA，1-2 天）

目標：在不訓 selector 的情況下，確認新 route source 是否有固定預算容量。

比較：

- nearest centroid；
- nearest-two；
- SOAR-style complementary secondary assignment；
- load-balanced route；
- route + low-DF lexical rescue。

每種 source 以 exact dense 與 frozen M392 scorer 分別 rerank，預算固定為
512/1k/2k/4k。

Phase 1 gate（2k 為主，4k 為最大容許診斷）：

- Dense O@100 `>= 0.98`；
- Recall@100 與 NDCG@10 保留 exact dense 的 `>= 99%`；
- CUB 不低於 dense control 的預設 floor；
- posting reads、candidate count、residual reads 完整可審計；
- 加入 nfcorpus/scifact canary 後沒有 catastrophic row。

Stop：

- 4k source oracle 仍無法過 O@100/Recall gate，關閉 route family；
- 只有 qrels oracle 能過、deterministic dense-derived source 完全不過，不進
  selector training。

### Phase 2：qrels-free query route selector（2-5 天）

前提：Phase 1 source oracle 通過。

訓練目標不是 benchmark ranking，而是 dense neighborhood distribution：

- teacher：dense topK route/candidate mass；
- objective：route assignment/distillation + load balance + query-weighted cost；
- corpus-derived train/heldout split；
- 不使用 BEIR qrels，不使用 dataset id；
- selector 只預測 routes/probes，不直接輸出 rerank score。

Gate：

- 保留 `>=95%` 的 deterministic/source oracle improvement；
- heldout corpus 不顯著退化；
- 固定 probe budget，不允許靠更多 route 偷渡成本；
- 若 learned selector 不如 deterministic multi-probe，保留 deterministic policy，
  不做更多 depth/loss grid。

### Phase 3：route-local native engine（2-4 天）

工作：

- 每個 route 建 residual-geometric blocks；
- ordinary BMP 作 control；
- exact bound mode 與 declared approximate mode 分開；
- 比較 int8-256、int4-256、PQ64B、PQ32B；
- 報 score parity、top100 boundary parity、p50/p95、bytes/doc、blocks opened。

Gate：

- exact mode 必須完整 parity；
- approximate mode quality retention `>=99%`，且標記為 approximate；
- 相對 exhaustive residual scoring 有實際 p95/bytes 改善；
- 若再次所有 blocks 打開，停止 exact block branch，不再換 block size/grid。

### Phase 4：同索引 lexical rescue（1-3 天）

前提：semantic route 已通過 Phase 1-3。

工作：

- 只添加 low-DF/high-utility lexical keys；
- lexical 與 route 使用不同 namespace，但在同一 physical index；
- 按 query-weighted posting work 限制成本；
- 評估 lexical unique candidates、O@256、CUB 與 row harm。

Gate：

- O@256/CUB 有可重複改善；
- NDCG/MRR 不因 rescue 顯著下降；
- 不允許重現 full-term `2.552668x` reads；
- 如果只有 high-DF/full-term 才有 coverage，保留其 oracle 結論但停止產品化。

### Phase 5：native corpus 擴大（3-7 天，視資源而定）

順序：

1. FiQA；
2. nfcorpus + scifact；
3. 一個 million-scale corpus；
4. official BEIR15 full corpus。

規則：

- 每一階段先過 source gate，再建完整 index；
- 遠端 GPU/CPU 節點只在空閒時使用，不干擾其他訓練；
- 每 dataset streaming build/eval，避免同時物化所有 full roots；
- 大型 index/result 可歸檔到 `/Volumes/Betty`，但遠端刪除必須在 checksum、
  manifest、readback 驗證後進行；
- full15 不做 dataset-specific tuning。

## 8. 最終驗收矩陣

所有結果至少比較：

1. BM25 native；
2. dense exact / VectorChord control；
3. historical M392 scorer ceiling；
4. semantic route only；
5. semantic route + M392 residual；
6. semantic route + residual + low-DF lexical rescue。

必報 metrics：

- NDCG@10；
- MAP@100；
- Recall@100；
- MRR@20；
- dense overlap O@10/O@100/O@256；
- candidate upper bound；
- posting entries read；
- unique candidates；
- blocks opened；
- residual bytes read；
- index bytes/doc；
- build time；
- p50/p95 latency；
- query-weighted work `W(q)`。

正式 promotion 必須同時滿足：

- fixed candidate budget，不使用 corpus ratio；
- one physical inverted index，不使用 VectorChord 作 candidate source；
- exact 或明確標註 approximate；
- canary O@100 `>=0.98`；
- Recall/NDCG 至少保留 dense/M392 ceiling 的 99%；
- broader/full native row safety；
- latency與空間相對對照有實際優勢，而不是只改善 offline touch proxy。

## 9. 全局停止條件

符合任何一項即停止相應分支，不再用 learning rate/loss sweep 延命：

1. deterministic source oracle 在 2k/4k 固定預算下失敗；
2. selector 無法在 heldout 保留 95% source-oracle improvement；
3. route-local geometric blocks 只能用超過 1% 的品質損失換取成本下降；
4. lexical rescue 只有打開 high-DF/full-term postings 才有作用；
5. full native latency/bytes 不優於 dense control，且品質更差；
6. 增益只存在於 sampled face、單 dataset 或 dataset-specific tuning；
7. source/checkpoint/projection identity 不可驗證。

若 Phase 1/2 失敗，M3xx 應正式永久歸檔為：

- 有效的 dense-tail scorer/capacity proof；
- 無法在固定成本下完成 admission 的產品負結果。

## 10. 不應再做的事情

- 不再直接重跑 M392 5%/8%/10% corpus-ratio sweep；
- 不再掃 BM25 alpha；
- 不在 source oracle 失敗時訓更深 selector；
- 不把 rerank set size 報成 index touch；
- 不用 raw posting union 單獨判斷所有表示的成本；
- 不把 sampled BEIR15 當 official full-corpus 結論；
- 不先訓 unified text encoder，再事後檢查 deterministic representation 是否有
  容量；
- 不把 ANN + BM25 當作 M3xx 統一倒排產品交付。

## 11. 暫定決策

M3xx 的正確重啟方式不是「復活 M392」，而是：

> 凍結 M392 tail scorer，把 signed-coordinate admission 換成 retrieval-oriented
> semantic routes；用 complementary secondary assignment、low-DF lexical rescue
> 與 route-local residual-geometric blocks，在固定 2k-4k candidate budget 下完成
> one-index native proof。

整體把握：

- 找到比 historical M392 更誠實的品質/成本 Pareto：**中等把握**；
- 在固定 2k 左右恢復 dense top100：**有 oracle 支持，值得測試**；
- 同時恢復 O@256 且保持低成本：**風險較高**；
- 最終得到單一 unified inverted index：**結構上可行，但尚未被本項目證明**。

因此建議在資源空閒時執行 Phase 0-1。只有 Phase 1 過 gate，才投入後續訓練與
engine 工程；否則立即關閉，不再消耗新的大規模訓練成本。

## 12. 主要內部證據

- `docs/research-sae/reports/m0300-m0399/ii42-m370-project-reset-review-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m377-m380-dense-preservation-next-stage-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m392-runtime-tail-bm25-index-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m392-beir15-standard-recall-matrix.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m394-m395-mteb-robustness-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m396-mteb-frontier-milestone.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1541-pooled-dense-posting-capacity-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1542-corpus-route-posting-capacity-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1565-fixed-budget-route-frontier-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1566-hi2-unified-source-report.md`
- `docs/research-sae/reports/m1600-m1699/ii42-m1630-exact-signed-block-engine-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1910-latent-terms-native-bmp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1912a-external-splare-native-bmp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1963-m190-bmp-engine-cost-attribution.md`

## 13. 相鄰文獻

- SOAR: <https://arxiv.org/abs/2404.00774>
- Distill-VQ: <https://arxiv.org/abs/2204.00185>
- RepCONC: <https://arxiv.org/abs/2110.05789>
- HI2: <https://arxiv.org/abs/2210.05521>
- CITADEL: <https://arxiv.org/abs/2211.10411>
- Seismic: <https://arxiv.org/abs/2404.18812>
- Block-Max Pruning: <https://arxiv.org/abs/2405.01117>
- Two-Step SPLADE: <https://arxiv.org/abs/2404.13357>
- Cluster-based approximate sparse retrieval: <https://arxiv.org/abs/2404.08896>
- DF-FLOPS: <https://arxiv.org/abs/2505.15070>
- Searching Dense Representations with Inverted Indexes:
  <https://arxiv.org/abs/2312.01556>
