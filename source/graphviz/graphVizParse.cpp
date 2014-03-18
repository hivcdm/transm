/*
 * graphVizParse.cpp
 *
 *  Created on: Sep 14, 2009
 *      Author: errhode
 */

#include <climits>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>

#include "graphVizParse.h"

/**GraphVizParse::GraphVizParse(int _timeSteps, std::string _simName){
	int timeStep;

	for (timeStep = 0; timeStep <= _timeSteps; timeStep++){
		std::string nodeFile(_simName + "-transGraphViz" + lexical_cast<std::string>(timeStep) + ".gv");
		std::string edgeFile("graphViz-" + _simName + "-allEdges.txt");

		//The regular expressions which will be used to match the edges.
		boost::regex TTi("TT" + lexical_cast<std::string>(timeStep) + "TT");
		boost::regex TTd("TT[0-9]+TT");

		//Open nodes outfile for timestep i
		std::ofstream nodesOut(nodeFile.c_str(), std::ios::app);

		//Open edges infile
		std::ifstream edgesIn(edgeFile.c_str());
		if (edgesIn.is_open()){
			std::string edgesLine;
			//For each line in EDGES
			while (!edgesIn.eof()) {
				getline(edgesIn, edgesLine);

				//Replace using regex
				edgesLine = regex_replace(edgesLine, TTi, "solid");
				edgesLine = regex_replace(edgesLine, TTd, "invis");

				//Print to NODES file using replaced line
				nodesOut << edgesLine << std::endl;
			}
		}

		//Close EDGES
		edgesIn.close();
		//Close NODES
		nodesOut.close();
	}
}*/

SexualPartnership::Type GraphVizGraphElements::relationshipEdge::statusAtTime(int time)
{
	/** Iterate through the timePairs and check if time falls between them */
	for(vector<timePair>::iterator timesIt = times.begin(); timesIt != times.end(); ++timesIt)
	{
		if(time >= (*timesIt).start && time <= (*timesIt).end)
		{
			/** If time is within a time pair, return the relationship type for that pair */
			return (*timesIt).relationshipType;
		}
	}

	/** If time did not fall between any time pairs, return the ENDType */
	return SexualPartnership::ENDType;
}

GraphVizGraphElements::personNode::personNode(int ID, bool _isMale, int timeBornAt)
{
	personID = ID;
	isMale = _isMale;
	timeBorn = timeBornAt;
	/** It is expected that timeDied and timeInfected will be updated when those events occur */
	timeDied = INT_MAX;
	timeInfected = INT_MAX;
	timeSA = INT_MAX;
}

bool GraphVizGraphElements::personNode::wasAlive(int time)
{
	return (time >= timeBorn && time <= timeDied);
}

bool GraphVizGraphElements::personNode::wasInfected(int time)
{
	return (wasAlive(time) && time >= timeInfected);
}

bool GraphVizGraphElements::personNode::wasBorn(int time)
{
	return (time >= timeBorn);
}

bool GraphVizGraphElements::personNode::wasSA(int time)
{
	return (time >= timeSA);
}

bool GraphVizGraphElements::personNode::hadDied(int time)
{
	return (time > timeDied);
}

void GraphVizGraphElements::personNode::addRelationship(unsigned long partnerID, int timeStart, int timeEnd,
        SexualPartnership::Type relationshipType)
{
	/** Form a TimePair for this relationship */
	timePair newTimePair;
	newTimePair.start = timeStart;
	newTimePair.end = timeEnd;
	newTimePair.relationshipType = relationshipType;

	/** Check if a relationship edge already exists with this partner */
	for(auto relationshipsIt = relationships.begin(); relationshipsIt != relationships.end(); ++relationshipsIt)
	{
		if(static_cast<unsigned long>((*relationshipsIt).partnerID) == partnerID)
		{
			/** If we previously formed a relationship with this partner, add the new time pair to the existing edge and be done */
			(*relationshipsIt).times.push_back(newTimePair);
			return;
		}
	}

	/** If there is no existing relationship, create a new relationship edge for this partnership and add it to the list of edges */
	relationshipEdge newRelation;
	newRelation.partnerID = partnerID;
	newRelation.times.push_back(newTimePair);
	relationships.push_back(newRelation);
}

void GraphVizGraphElements::printGraphVizFiles(int _timeSteps, std::string _simName)
{
	for(int timeToGraph = 0; timeToGraph <= _timeSteps; timeToGraph++)
	{
		std::string timeToGraphString(boost::lexical_cast<std::string>(timeToGraph));

		//Add the appropriate number of 0s to make it a three character string
		if(timeToGraph < 100)
		{
			timeToGraphString.insert(0, "0");
		}

		if(timeToGraph < 10)
		{
			timeToGraphString.insert(0, "0");
		}

		std::string filename(_simName + "-GraphViz" + timeToGraphString + ".gv");
		std::ofstream outstream(filename.c_str(), std::ios::out);
		/** Print the graphViz standard header information */
		outstream << "Digraph world {" << std::endl;
		outstream << "graph[size=\"10,10\",ratio=fill,pack=1,center=1];" << std::endl;
		outstream << "node[style=filled,label=\"\"];" << std::endl;

		/** Print the nodes (persons) in the graph */
		for(vector<personNode *>::iterator nodeIt = persons.begin(); nodeIt != persons.end(); ++nodeIt)
		{
			//Determine the shape based on the gender of the node owner
			if((*nodeIt)->isMale)
			{
				outstream << "node[shape=box,";
			}
			else
			{
				outstream << "node[shape=triangle,";
			}

			/** Determine the color based on whether or not the person is not yet born (white), alive and uninfected (green), infected (red), or dead (gray) */
			if(!(*nodeIt)->wasBorn(timeToGraph))
			{
				outstream << "color=white]; ";
			}
			else if((*nodeIt)->hadDied(timeToGraph))
			{
				outstream << "color=gray75]; ";
			}
			else if(!(*nodeIt)->wasSA(timeToGraph))
			{
				outstream << "color=yellow]; ";
			}
			else if((*nodeIt)->wasInfected(timeToGraph))
			{
				outstream << "color=red]; ";
			}
			else
			{
				outstream << "color=green]; ";
			}

			//Print the node value
			outstream << (*nodeIt)->personID << ";" << std::endl;
		}

		/** Print the edges in the graph */
		/** Mark the edges as having weight 3 for visibility */
		outstream << "edge[penwidth=3];" << std::endl;

		for(vector<personNode *>::iterator nodeIt = persons.begin(); nodeIt != persons.end(); ++nodeIt)
		{
			for(vector<relationshipEdge>::iterator relsIt = (*nodeIt)->relationships.begin();
			        relsIt != (*nodeIt)->relationships.end(); ++relsIt)
			{
				int headNode = (*nodeIt)->personID;
				int tailNode = (*relsIt).partnerID;
				SexualPartnership::Type pType = (*relsIt).statusAtTime(timeToGraph);
				std::string style(pType == SexualPartnership::ENDType ? "invis" : "solid");
				std::string color("");

				//Get the color based on the relationship type
				if(pType == SexualPartnership::CASUAL)
				{
					color.append("darkgreen");
				}
				else if(pType == SexualPartnership::CSW)
				{
					color.append("firebrick");
				}
				else if(pType == SexualPartnership::REGULAR)
				{
					color.append("darkviolet");
				}
				else    //STEADY or invisible
				{
					color.append("blue");
				}

				outstream << headNode << " -> " << tailNode << "[color=" << color << ",style=" << style << "];" << std::endl;
			}
		}

		/** Print the closing lines */
		outstream << "overlap = prism1000" << std::endl << "}" << std::endl;
		/** Close the outstream */
		outstream.close();
	}
}
