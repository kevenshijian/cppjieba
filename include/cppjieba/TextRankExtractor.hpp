﻿#ifndef CPPJIEBA_TEXTRANK_EXTRACTOR_H
#define CPPJIEBA_TEXTRANK_EXTRACTOR_H

#include <cmath>
#include "Jieba.hpp"

namespace cppjieba {
	using namespace limonp;
	using namespace std;

	class TextRankExtractor {
	public:
		typedef struct _Word {string word;vector<size_t> offsets;double weight;}    Word; // struct Word
	private:
		typedef std::unordered_map<string,Word> WordMap;
	
		class WordGraph{
		private:
			typedef double Score;
			typedef string Node;
			typedef std::unordered_set<Node> NodeSet;
			typedef std::unordered_map<Node,double> Edges;
			typedef std::unordered_map<Node,Edges> Graph;

			double d;
			Graph graph;
			NodeSet nodeSet;
		public:
			WordGraph(): d(0.85) {};
			WordGraph(double in_d): d(in_d) {};

			void addEdge(Node start,Node end,double weight){
				Edges temp;
				Edges::iterator gotEdges;
				nodeSet.insert(start);
				nodeSet.insert(end);
				graph[start][end]+=weight;
				graph[end][start]+=weight;
			}

			void rank(WordMap &ws,size_t rankTime=10){
				WordMap outSum;
				Score wsdef, min_rank, max_rank;

				if( graph.size() == 0)
					return;

				wsdef = 1.0 / graph.size();

				for(Graph::iterator edges=graph.begin();edges!=graph.end();++edges){
					// edges->first start节点；edge->first end节点；edge->second 权重
					ws[edges->first].word=edges->first;
					ws[edges->first].weight=wsdef;
					outSum[edges->first].weight=0;
					for(Edges::iterator edge=edges->second.begin();edge!=edges->second.end();++edge){
						outSum[edges->first].weight+=edge->second;
					}
				}
				//sort(nodeSet.begin(),nodeSet.end()); 是否需要排序?
				for( size_t i=0; i<rankTime; i++ ){
					for(NodeSet::iterator node = nodeSet.begin(); node != nodeSet.end(); node++ ){
						double s = 0;
						for( Edges::iterator edge= graph[*node].begin(); edge != graph[*node].end(); edge++ )
							// edge->first end节点；edge->second 权重
							s += edge->second / outSum[edge->first].weight * ws[edge->first].weight;
						ws[*node].weight = (1 - d) + d * s;
					}
				}

				min_rank=max_rank=ws.begin()->second.weight;
				for(WordMap::iterator i = ws.begin(); i != ws.end(); i ++){
					if( i->second.weight < min_rank ){
						min_rank = i->second.weight;
					}
					if( i->second.weight > max_rank ){
						max_rank = i->second.weight;
					}
				}
				for(WordMap::iterator i = ws.begin(); i != ws.end(); i ++){
					ws[i->first].weight = (i->second.weight - min_rank / 10.0) / (max_rank - min_rank / 10.0);
				}
			}
		};

	public: 
		TextRankExtractor(const Jieba& jieba, const string& stopWordPath) : segment_(jieba.GetDictTrie(), jieba.GetHMMModel()) {
				LoadStopWordDict(stopWordPath);
		}
		~TextRankExtractor() {
		}

		void Extract(const string& sentence, vector<Word>& keywords, size_t topN, size_t span=5,size_t rankTime=10) const {
			vector<string> words;
			segment_.Cut(sentence, words);

			TextRankExtractor::WordGraph graph;
			WordMap wordmap;
			size_t offset = 0;

			for(size_t i=0; i < words.size(); i++){
				size_t t = offset;
				offset += words[i].size();
				if (IsSingleWord(words[i]) || stopWords_.find(words[i]) != stopWords_.end()) {
					continue;
				}
				for(size_t j=i+1;j<i+span && j<words.size();j++){
					if (IsSingleWord(words[j]) || stopWords_.find(words[j]) != stopWords_.end()) {
						continue;
					}
					graph.addEdge(words[i],words[j],1);
				}
				wordmap[words[i]].offsets.push_back(t);
			}
			if (offset != sentence.size()) {
				XLOG(ERROR) << "words illegal";
				return;
			}

			graph.rank(wordmap,rankTime);
			
			keywords.clear();
			keywords.reserve(wordmap.size());
			for (WordMap::iterator itr = wordmap.begin(); itr != wordmap.end(); ++itr) {
				keywords.push_back(itr->second);
			}
			
			topN = min(topN, keywords.size());
			partial_sort(keywords.begin(), keywords.begin() + topN, keywords.end(), Compare);
			keywords.resize(topN);
		}
	private:
		void LoadStopWordDict(const string& filePath) {
			ifstream ifs(filePath.c_str());
			XCHECK(ifs.is_open()) << "open " << filePath << " failed";
			string line ;
			while (getline(ifs, line)) {
				stopWords_.insert(line);
			}
			assert(stopWords_.size());
		}

		bool IsSingleWord(const string& str) const {
			Unicode unicode;
			TransCode::Decode(str, unicode);
			if (unicode.size() == 1)
				return true;
			return false;
		}

		static void sortMapValue(WordMap &map,vector<Word>& result,size_t topN){
			for(auto i=map.begin();i!=map.end();i++){
				result.push_back(i->second);
			}
			partial_sort(result.begin(),result.begin()+topN,result.end(),Compare);
		}

		static bool Compare(const Word &x,const Word &y){
			return x.weight > y.weight;
		}

		MixSegment segment_;
		unordered_set<string> stopWords_;
	};
} // namespace cppjieba

#endif

