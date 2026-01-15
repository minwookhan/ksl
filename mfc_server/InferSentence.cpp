#include "InferSentence.h"



void InferSentence::BuildCorpusGlossWeight(std::vector<_typeCorpusVideoDB> &CorpusVideoDB)
{
	auto& corpusDB = CorpusVideoDB;
	const size_t N = corpusDB.size();
	if (N == 0) return;

	// 1) gloss 별 docFreq 계산 (몇 개 문장(corpus)에 등장하는지)
	std::unordered_map<UINT32, int> docFreq; // sgIndex -> 등장 corpus 수

	for (auto& corpus : corpusDB)
	{
		std::unordered_set<UINT32> uniqueSet;

		if (!corpus.scv_unified_sgIndex.empty())
		{
			uniqueSet.insert(corpus.scv_unified_sgIndex.begin(),
				corpus.scv_unified_sgIndex.end());
		}
		else
		{
			uniqueSet.insert(corpus.scv_sgIndex.begin(),
				corpus.scv_sgIndex.end());
		}

		// corpus 내부 unique gloss 리스트 저장
		corpus.scv_gloss_unique.assign(uniqueSet.begin(), uniqueSet.end());

		// docFreq 누적
		for (UINT32 g : uniqueSet)
			docFreq[g]++;
	}

	// 2) IDF 기반 weight 계산
	const double Ndouble = static_cast<double>(N);

	for (auto& corpus : corpusDB)
	{
		const auto& G = corpus.scv_gloss_unique;
		corpus.scv_gloss_unique_weight.resize(G.size());

		for (size_t i = 0; i < G.size(); ++i)
		{
			UINT32 g = G[i];
			int df = 0;
			auto it = docFreq.find(g);
			if (it != docFreq.end()) df = it->second;

			// df 작을수록(희소) 값 크게, df 클수록 작게
			// (N+1)/(df+0.5) 는 smoothing 용
			double idf = std::log((Ndouble + 1.0) / (df + 0.5));

			if (idf < 0.0) idf = 0.0;  // 방어

			corpus.scv_gloss_unique_weight[i] = idf;
		}
	}
}
//_typeCorpusVideoDB InferSentence::InferCorpus(std::vector<GlossInfered> inferList,
//    std::vector<_typeCorpusVideoDB>& CorpusVideoDB)
//{
//    _typeCorpusVideoDB bestCorpus;
//
//    if (inferList.empty() || CorpusVideoDB.empty())
//        return bestCorpus;
//
//    // ===== 1) infer 결과에서 sgIndex별 집계 + 전체 gloss 집합 =====
//    struct AggInfo
//    {
//        double sumWeightedProb = 0.0; // (prob * ORDER_WEIGHT) 합
//        int    count = 0;             // 등장 횟수 (프레임 수, 후보 수)
//    };
//
//    std::unordered_map<int, AggInfo> aggMap;      // MIN_SIMILARITY 이상 gloss용
//    std::unordered_set<int> allInferGlossSet;     // prob 상관없이 infer에 등장한 모든 gloss
//
//    for (const auto& g : inferList)
//    {
//        // infer_order 범위 체크
//        if (g.infer_order < 0 || g.infer_order > 2)
//            continue;
//
//        // 모든 gloss는 "어떤 문장에 포함되어 있는지" 판단에는 사용
//        allInferGlossSet.insert(g.sgIndex);
//
//        // MIN_SIMILARITY 이하는 점수/카운트 집계에서는 제외
//        if (g.prob <= MIN_SIMILARITY)
//            continue;
//
//        double wProb = g.prob * ORDER_WEIGHT[g.infer_order];
//
//        AggInfo& info = aggMap[g.sgIndex];
//        info.sumWeightedProb += wProb;
//        info.count += 1;  // 이게 "후보 글로스 수" 역할
//    }
//
//    // infer에 나온 gloss 자체가 없으면 그냥 빈 결과
//    if (allInferGlossSet.empty())
//        return bestCorpus;
//
//    // ===== 2) sgIndex → score(평균 weighted prob) 맵 (tie-break 용) =====
//    std::unordered_map<int, double> sgScore;
//
//    for (const auto& kv : aggMap)
//    {
//        int sgIndex = kv.first;
//        const AggInfo& info = kv.second;
//
//        if (info.count <= 0)
//            continue;
//
//        double avgProb = info.sumWeightedProb / static_cast<double>(info.count);
//        sgScore[sgIndex] = avgProb;
//    }
//
//    // ===== 3) 문장 선택 로직 =====
//    //  - 1순위: "문장을 구성하는 모든 gloss 가 infer에 포함되는 문장(full match)"
//    //           그 중 고유 gloss 개수가 많은 문장 우선
//    //           (동점이면 글로스 매칭 수(count 합)가 많은 문장 우선)
//    //  - full match 가 하나도 없으면:
//    //           부분 매칭 문장들 중에서, 겹치는 gloss 개수가 많은 문장 우선
//    //           (동점이면 글로스 매칭 수(count 합) 우선)
//    //
//    //  여기서 "글로스 매칭 수" = infer에서 해당 gloss가 등장한 총 횟수(count)의 합
//
//    // full match 후보(best)
//    bool   hasFullBest = false;
//    int    bestFullGlossCnt = -1;   // 고유 gloss 개수 (uniqueGloss.size())
//    int    bestFullSupport = -1;   // 글로스 매칭 수 합( count 합 )
//    double bestFullScore = -1.0; // gloss score 합
//    size_t bestFullSeqLen = 0;    // 전체 시퀀스 길이 (동점이면 짧은 문장 우선)
//    _typeCorpusVideoDB bestFullCorpus;
//
//    // partial match 후보(best)
//    bool   hasPartBest = false;
//    int    bestPartOverlap = -1;   // infer와 겹치는 고유 gloss 개수
//    int    bestPartSupport = -1;   // 그 겹친 gloss들의 count 합
//    double bestPartScore = -1.0; // 겹치는 gloss score 합
//    size_t bestPartSeqLen = 0;    // 전체 시퀀스 길이
//    _typeCorpusVideoDB bestPartCorpus;
//
//    const double eps = 1e-12;
//
//    for (const auto& corpus : CorpusVideoDB)
//    {
//        const std::vector<UINT32>& corpusSeq = corpus.scv_sgIndex;
//        if (corpusSeq.empty())
//            continue;
//
//        // 문장 내 고유 gloss 집합
//        std::unordered_set<UINT32> uniqueGloss(corpusSeq.begin(), corpusSeq.end());
//        const size_t corpusGlossCount = uniqueGloss.size();
//        if (corpusGlossCount == 0)
//            continue;
//
//        // 이 문장에서 infer에 잡힌 gloss가 몇 개나 겹치는지 계산
//        int    overlapCount = 0;   // 겹치는 고유 gloss 개수
//        int    overlapSupport = 0;   // 그 gloss들의 count 합 (글로스 매칭 수)
//        double overlapScore = 0.0; // 그 gloss들의 score 합
//
//        for (UINT32 sg : uniqueGloss)
//        {
//            int sgInt = static_cast<int>(sg);
//
//            // infer에 한 번이라도 등장한 gloss이면 overlap
//            if (allInferGlossSet.find(sgInt) != allInferGlossSet.end())
//            {
//                overlapCount++;
//
//                // support: aggMap에 있으면 count(등장 횟수)를 더함
//                auto itAgg = aggMap.find(sgInt);
//                if (itAgg != aggMap.end())
//                    overlapSupport += itAgg->second.count;
//
//                // score: MIN_SIMILARITY 이상 gloss만 sgScore에서 가져옴
//                auto itScore = sgScore.find(sgInt);
//                if (itScore != sgScore.end())
//                    overlapScore += itScore->second;
//            }
//        }
//
//        if (overlapCount <= 0)
//            continue; // 이 문장은 infer와 전혀 안 겹침
//
//        const size_t seqLen = corpusSeq.size(); // 전체 시퀀스 길이
//
//        // ----- full match 여부: 문장을 구성하는 모든 gloss 가 infer에 포함되는 경우 -----
//        bool isFull = (overlapCount == static_cast<int>(corpusGlossCount));
//
//        if (isFull)
//        {
//            // === full match 후보들 중 best 선택 ===
//            bool isBetterFull = false;
//
//            // ① 고유 gloss 개수(문장을 구성하는 gloss 수)가 많은 문장 우선
//            if (static_cast<int>(corpusGlossCount) > bestFullGlossCnt)
//            {
//                isBetterFull = true;
//            }
//            else if (static_cast<int>(corpusGlossCount) == bestFullGlossCnt)
//            {
//                // ② 글로스 매칭 수(count 합)가 많은 문장 우선
//                if (overlapSupport > bestFullSupport)
//                {
//                    isBetterFull = true;
//                }
//                else if (overlapSupport == bestFullSupport)
//                {
//                    // ③ score 합(확률 기반) 큰 문장
//                    if (overlapScore > bestFullScore + eps)
//                    {
//                        isBetterFull = true;
//                    }
//                    else if (std::fabs(overlapScore - bestFullScore) <= eps)
//                    {
//                        // ④ 전체 길이가 짧은 문장 우선
//                        if (!hasFullBest || seqLen < bestFullSeqLen)
//                            isBetterFull = true;
//                    }
//                }
//            }
//
//            if (isBetterFull || !hasFullBest)
//            {
//                hasFullBest = true;
//                bestFullGlossCnt = static_cast<int>(corpusGlossCount);
//                bestFullSupport = overlapSupport;
//                bestFullScore = overlapScore;
//                bestFullSeqLen = seqLen;
//                bestFullCorpus = corpus;
//            }
//        }
//        else
//        {
//            // === partial match 후보들 중 best 선택 ===
//            bool isBetterPart = false;
//
//            // ① infer와 겹치는 고유 gloss 개수(overlapCount)가 많은 문장 우선
//            if (overlapCount > bestPartOverlap)
//            {
//                isBetterPart = true;
//            }
//            else if (overlapCount == bestPartOverlap)
//            {
//                // ② 글로스 매칭 수(count 합)가 많은 문장 우선
//                if (overlapSupport > bestPartSupport)
//                {
//                    isBetterPart = true;
//                }
//                else if (overlapSupport == bestPartSupport)
//                {
//                    // ③ 겹치는 gloss score 합 큰 문장
//                    if (overlapScore > bestPartScore + eps)
//                    {
//                        isBetterPart = true;
//                    }
//                    else if (std::fabs(overlapScore - bestPartScore) <= eps)
//                    {
//                        // ④ 전체 길이가 짧은 문장 우선
//                        if (!hasPartBest || seqLen < bestPartSeqLen)
//                            isBetterPart = true;
//                    }
//                }
//            }
//
//            if (isBetterPart || !hasPartBest)
//            {
//                hasPartBest = true;
//                bestPartOverlap = overlapCount;
//                bestPartSupport = overlapSupport;
//                bestPartScore = overlapScore;
//                bestPartSeqLen = seqLen;
//                bestPartCorpus = corpus;
//            }
//        }
//    }
//
//    // ===== 최종 선택 =====
//    if (hasFullBest)
//    {
//        // full match 문장이 하나라도 있으면 그 중 best 반환
//        bestCorpus = bestFullCorpus;
//    }
//    else if (hasPartBest)
//    {
//        // full match 가 전혀 없으면 partial match 중 best 반환
//        bestCorpus = bestPartCorpus;
//    }
//    else
//    {
//        // infer와 한 글로스도 겹치지 않는 경우만 존재 → bestCorpus 는 초기값(빈) 반환
//    }
//
//    return bestCorpus;
//}

_typeCorpusVideoDB InferSentence::InferCorpus(std::vector<GlossInfered> inferList,
    std::vector<_typeCorpusVideoDB>& CorpusVideoDB)
{
    _typeCorpusVideoDB emptyCorpus;
    _typeCorpusVideoDB bestCorpus;

    if (inferList.empty() || CorpusVideoDB.empty())
        return emptyCorpus;

    // ===== 1) inferList → sgIndex 등장 횟수 집계 =====
    std::unordered_map<int, int> sgCount;
    int totalCount = 0;

    for (const auto& g : inferList)
    {
        // 필요하면 infer_order 체크
        if (g.infer_order < 0 || g.infer_order > 2)
            continue;

        // MIN_SIMILARITY 필터 없이, 등장한 것은 모두 반영
        sgCount[g.sgIndex] += 1;
        totalCount += 1;
    }

    if (sgCount.empty() || totalCount <= 0)
        return emptyCorpus;

    const double eps = 1e-12;

    // ===== 2) 문장들을 2개 이상 매칭 group / 1개 매칭 group 으로 나누기 =====
    struct SentInfo
    {
        const _typeCorpusVideoDB* corpus;
        double  prob;              // P(S)
        size_t  glossCount;        // 문장의 고유 gloss 수
        size_t  seqLen;            // scv_sgIndex 전체 길이
        int     matchedGlossCount; // infer에 실제로 매칭된 gloss 개수
    };

    std::vector<SentInfo> group2plus; // matchedGlossCount >= 2
    std::vector<SentInfo> group1;     // matchedGlossCount == 1

    for (const auto& corpus : CorpusVideoDB)
    {
        const auto& seq = corpus.scv_sgIndex;
        if (seq.empty())
            continue;

        std::unordered_set<UINT32> uniqueGloss(seq.begin(), seq.end());
        size_t glossCount = uniqueGloss.size();
        if (glossCount == 0)
            continue;

        // --- P(S) 및 matchedGlossCount 계산 ---
        double sentenceProb = 0.0;
        int matchedGlossCount = 0;

        for (UINT32 sg : uniqueGloss)
        {
            auto it = sgCount.find(static_cast<int>(sg));
            if (it == sgCount.end())
                continue; // infer에 한 번도 안 나온 gloss

            matchedGlossCount++;

            double p = static_cast<double>(it->second) /
                static_cast<double>(totalCount);
            if (p > sentenceProb)
                sentenceProb = p;
        }

        // infer와 전혀 안 겹치면 후보에서 제외
        if (matchedGlossCount <= 0 || sentenceProb <= 0.0)
            continue;

        SentInfo info;
        info.corpus = &corpus;
        info.prob = sentenceProb;
        info.glossCount = glossCount;
        info.seqLen = seq.size();
        info.matchedGlossCount = matchedGlossCount;

        if (matchedGlossCount >= 2)
            group2plus.push_back(info);
        else // matchedGlossCount == 1
            group1.push_back(info);
    }

    // 두 그룹이 모두 비면 선택할 문장 없음
    if (group2plus.empty() && group1.empty())
        return emptyCorpus;

    // ===== 3) 어떤 그룹에서 고를지 결정 (2개 이상 매칭 우선) =====
    const std::vector<SentInfo>* targetGroup = nullptr;
    if (!group2plus.empty())
        targetGroup = &group2plus;
    else
        targetGroup = &group1;  // 2개 이상 매칭 문장이 없을 때만 1개 매칭 문장들 비교

    // ===== 4) targetGroup 내에서 최종 문장 선택 =====
    bool   hasBest = false;
    double bestProb = -1.0;
    size_t bestGlossCount = 0;
    size_t bestSeqLen = 0;

    for (const auto& s : *targetGroup)
    {
        bool isBetter = false;

        // ① P(S) 큰 문장
        if (!hasBest || s.prob > bestProb + eps)
        {
            isBetter = true;
        }
        else if (std::fabs(s.prob - bestProb) <= eps)
        {
            // ② P(S) 같으면 → glossCount 적은 문장
            if (s.glossCount < bestGlossCount)
            {
                isBetter = true;
            }
            else if (s.glossCount == bestGlossCount)
            {
                // ③ 그래도 같으면 → seqLen 짧은 문장
                if (!hasBest || s.seqLen < bestSeqLen)
                    isBetter = true;
            }
        }

        if (isBetter)
        {
            hasBest = true;
            bestProb = s.prob;
            bestGlossCount = s.glossCount;
            bestSeqLen = s.seqLen;
            bestCorpus = *(s.corpus);
        }
    }

    if (!hasBest)
        return emptyCorpus;

    return bestCorpus;
}



