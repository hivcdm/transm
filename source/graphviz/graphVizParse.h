/*
 * graphViz.h
 *
 *  Created on: Sep 14, 2009
 *      Author: errhode
 */

#ifndef GRAPHVIZPARSE_H_
#define GRAPHVIZPARSE_H_

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>

#include "../entities/classifiers/SexualPartnership.h"

/** class GraphVizParse {

public :
	Performs the final preprocessing of graphViz files by appending the edges to the node files for each time step with
	the appropriate edge weights (solid for current time, invis for other times)
	@param _inputFile path to CEPAC file
	@author errhode
	GraphVizParse(int _timeSteps, std::string _simName);

}; */


class GraphVizGraphElements {
public:
	/**
	 * A small class of two integers, start and end, indicating the duration of a pairing and the type of relationship during this period
	 */
	class timePair {
	public:
		/** The time period the relationship started */
		int start;
		/** The time period the relationship ended */
		int end;
		/** The type of relationship formed during this period */
		SexualPartnership::Type relationshipType;
	};

	/**
	 * A relationshipEdge should belong to a (male) node.  It contains the (female) node the original node had a relationship with a list of the time periods in which they had a relationship.
	 */
	class relationshipEdge {
	public:
		/** The ID of the person the owner node had a partnership with */
		int partnerID;
		/** A list of the time periods partner and the owner nodes were in a relationship */
		std::vector<timePair> times;

		/**
		 * @param time the time to check what the status was
		 * @return SexualPartnership::Type indicating what type of sexual partnership this relationship was involved in at the given time or SexualPartnership::ENDType if no relationship existed
		 */
		SexualPartnership::Type statusAtTime(int time);
	};

	/**
	 * A personNode contains the ID of the person, the time the person was born (or entered the model), the time the person was infected (if ever), and the time the person died (or left the model).
	 * It also includes a list of the person's relationships if the person is male.
	 */
	class personNode {
	public:
		/** The ID of the person who "owns" this node */
		unsigned long personID;
		/** True if the owner of the node is a male */
		bool isMale;
		/** A list of the person's relationships (should be empty if the person is female) */
		std::vector<relationshipEdge> relationships;
		/** The time a person was born or the beginning of the model if they started alive */
		int timeBorn;
		/** The time a person became sexually active */
		int timeSA;
		/** The time a person was infected or the maxTime of the simulation if the person ended the simulation uninfected */
		int timeInfected;
		/** The time a person died or the maxTime of the simulation if the person ended the simulation still alive */
		int timeDied;

		/**
		 * Constructor
		 */
		personNode(int ID, bool _isMale, int timeBornAt);


		/**
		 * @param time the time to check if the person was alive
		 * @return true if the person was alive at the given time
		 */
		bool wasAlive(int time);

		/**
		 * @param time the time to check if the person had been born yet
		 * @return true if the person was already born at the given time
		 */
		bool wasBorn(int time);

		/**
		 * @param time the time to check if the person had been born yet
		 * @return true if the person was sexually active at the given time
		 */
		bool wasSA(int time);

		/**
		 * @param time the time to check if the person was alive
		 * @return true if the person had already died at the given time
		 */
		bool hadDied(int time);

		/**
		 * @param time the time to check if the person was infected
		 * @return true if the person was infected at the given time
		 */
		bool wasInfected(int time);

		/**
		 * Adds a relationship to this node's list of relationships
		 * First checks if a previous relationship was formed with this partner and if so, adds a new TimePair to that edge.
		 * Otherwise forms a new relationship edge.
		 *
		 * @param partnerID the ID of the new partner
		 * @param timeStart the time step the relationship started
		 * @param timeEnd the time step the relationship ended
		 * @param relationshipType the type of relationship formed
		 */
		void addRelationship(unsigned long partnerID, int timeStart, int timeEnd, SexualPartnership::Type relationshipType);
	};

	/** A vector of all person nodes making up the graph.  Male persons store all of their own edges.  */
	vector<personNode*> persons;

	//TODO: A function that prints out GraphViz data
	/**
	 * This function prints out GraphViz compatible files from the time dependent graph of relationships built over the course of the simulation
	 * @param _timeSteps the number of time steps to print a graphviz file for
	 */
	void printGraphVizFiles(int _timeSteps, std::string _simName);
};

#endif /* GRAPHVIZPARSE_H_ */
