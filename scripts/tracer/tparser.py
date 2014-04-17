"""
Follows transmission model simulation from trace file.
"""

# The following apparent "double" import is a hack to get around a known
# py2exe problem.  See
# http://www.py2exe.org/index.cgi/WorkingWithVariousPackagesAndModules
from xml.etree import ElementTree as ET
from xml.etree import cElementTree as ET

import os
import re
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.path import Path
import matplotlib.patches as patches
import warnings


class Partnership:
    """A partnership between two individuals.

    Currently this is a man and woman, but it really could be changed
    to be any two individuals.

    Attributes
    ----------
    person1, person2: Person objects
        The two individuals in the partnership.
    start, duration:  numeric scalar
        Denotes the starting time and duration of the partnership.
    num_acts:  numeric scalar
        Number of sex acts during partnership.
    alive:  logical scalar
        Whether or not the partnership is current or not.
    partnership_type : str
        One of 'CSW', 'Regular', 'Steady', 'Casual'
    """
    def __init__(self, person1=None, person2=None, partnership_type=None,
                 duration=None, start=-1):
        self.person1 = person1
        self.person2 = person2
        self.partnership_type = partnership_type
        self.duration = duration
        self.start = start
        self.num_acts = 0
        self.alive = True

        self.person1.add_partnership(self)
        self.person2.add_partnership(self)

    def __str__(self):
        msg = "{0} {1} partnered with {2} {3} ({4}, {5} RISK) "
        msg += "starting at {6} of length {7}.\n"
        msg = msg.format(self.person1.gender,
                         self.person1.person_id,
                         self.person2.gender,
                         self.person2.person_id,
                         self.partnership_type,
                         self.person2.risk,
                         self.start,
                         self.duration)
        return msg

    def partner_of(self, that_person):
        """Return the partner of the given individual."""
        if self.person1 is that_person:
            return self.person2
        else:
            return self.person1


class Person:
    """
    Attributes
    ----------
    id : int
       Numeric identifier of this person.
    age : int
       Age in months upon introduction to the simulation.
    csw : bool
       True if the individual is currently a CSW
    csw_ever : bool
       True if the individual is ever a CSW
    hiv_positive : bool
        True if HIV positive, otherwise False.
    infected_by : int
        ID of person who infected this person.
    infected : list
        IDs of people that this person infected.
    transmission_coefficient:  list
        tuples of timestep and transmission coeffients
    """
    def __init__(self, person_id):
        self.person_id = person_id
        self.csw = False
        self.ever_csw = False
        self.partnerships = []
        self.alive = True
        self.hiv_positive = False
        self.infected_by = -1
        self.infected = []
        self.time_of_infection = -1
        self.time_of_death = -1
        self.transmission_coefficient = []
        self.traced = False

        # To be set by subclass instance.
        self.gender = None
        self.risk = None
        self.age = None
        self.single = None

    def __str__(self):
        msg = "============\n"
        msg += "ID:  {0}"
        msg += "\n    Gender:  {1}"
        msg += "\n    CSW:  {2}"
        msg += "\n    Ever CSW:  {3}"
        msg += "\n    Single:  {4}"
        msg += "\n    Age:  {5} (months)"
        msg += "\n    Risk:  {6}"
        msg += "\n    HIV Positive:  {7}"
        msg += '\n    Number of partnerships:  {8}'
        msg = msg.format(self.person_id, self.gender, self.csw, self.ever_csw,
                         self.single, self.age, self.risk, self.hiv_positive,
                         len(self.partnerships))

        if self.hiv_positive:
            msg += "\n    Infected at:  {0}".format(self.time_of_infection)
        if not self.alive:
            msg += '\n        Died at timestep {0}.'.format(self.time_of_death)
        msg += '\n\n'
        return msg

    def set_transmission_coefficient(self, timestep, value):
        """Add transmission coefficient recorded during an act.

        Parameters
        ----------
        timestep : int
            Model timestep at which the act ocurred.
        value : float
            Recorded value of transmission coefficient for the act.
        """
        self.transmission_coefficient.append((timestep, value))

    def set_hiv_infection(self, timestep):
        """This person is now HIV positive.  Record the circumstances.

        Parameters
        ----------
        timestep : int
            Model timestep at HIV infection took place.
        """
        self.hiv_positive = True
        self.time_of_infection = timestep

    def add_partnership(self, partnership):
        """Record the partnership for this person.
        """
        self.partnerships.append(partnership)

    def max_concurrent(self, reltype):
        """Determine max of concurrent relationships of the given type.

        Determine the largest number of relationships of the given
        type at the same time over the course of the simulation.

        Parameters
        ----------
        reltype : str
            One of 'CSW', 'Regular', 'Casual', or 'Steady'.
        """

        if len(self.partnerships) == 0:
            return 0

        maxtime = max([x.start for x in self.partnerships])
        maxval = 0

        for time in range(maxtime+1):
            # The final time comparison, we back off a tiny bit because
            # we don't want a relationship that ends at t=t1 and another
            # relationship that starts at t=t1 to count as overlapping.
            parts = [x for x in self.partnerships
                     if x.partnership_type == reltype
                     and (x.start <= time)
                     and (time < (x.start + x.duration + 0.1))]
            maxval = max(maxval, len(parts))
        return maxval

    def plot(self, fig):
        """Plot the partnerships over the course of the simulation.

        Parameters
        ----------
        visible : bool
            Set to False only when not using the GUI.
        figsize : tuple
            If not provided, chooses matplotlib defaults.  Should only be used
            by the GUI.
        """
        if len(self.partnerships) == 0:
            msg = "No partnerships for individual {0}.".format(self.person_id)
            raise RuntimeError(msg)

        # Get an idea of how many different plotting y-levels we need
        # in order for each relationship type to nicely fit.
        reltypes = ['CSW', 'Casual', 'Regular', 'Steady']
        reltypes_nums = [self.max_concurrent(y) for y in reltypes]
        reltype_max = dict(zip(reltypes, reltypes_nums))

        # Set up the plot axis extents.
        min_t = min([x.start for x in self.partnerships])
        max_t = max([x.start for x in self.partnerships])
        min_y = 0
        max_y = sum(reltype_max.values())

        ax1 = fig.add_subplot(111)
        
        plt.xlim((min_t, max_t))
        plt.ylim((min_y, max_y + 1))

        # Clear the yticks until we know what we need.
        ax1.set_yticks([])

        # Plot each partnership type.
        self._plot_steady()
        self._plot_regular(reltype_max)
        self._plot_casual(reltype_max)
        self.plot_csw()

        # If this individual became HIV-positive, graph that as well.
        if self.hiv_positive and self.time_of_infection <= max_t:
            xcoords = [self.time_of_infection, self.time_of_infection]
            ycoords = plt.ylim()
            plt.plot(xcoords, ycoords, 'r-*')

        title = '{0} {1}'.format(self.gender, self.person_id)
        ax1.set_title(title, position=(0.5, 1.05))

        return ax1

    def plot_csw(self):
        """Plot CSW partnerships."""
        gca = plt.gca()
        xlim = gca.get_xlim()
        ylim = gca.get_ylim()
        max_y = ylim[1] - 1

        csw_partnerships = [x for x in self.partnerships
                            if x.partnership_type == 'CSW']
        if len(csw_partnerships) == 0:
            # If no CSW partnerships, then go no further.
            return

        # Go through each casual partnership.  If more than one
        # such partnership during a given timestep, we need to
        # stack them.
        for time in np.arange(xlim[1]):
            current_csws = [x for x in csw_partnerships if x.start == time]

            for j, partnership in enumerate(current_csws):
                # Was the partner HIV-positive at that time?
                partner = partnership.partner_of(self)
                ylevel = max_y - j
                artist, = plt.plot(time, ylevel, 'gx')
                artist.set_label('CSW')
                if partner.hiv_positive and partner.time_of_infection <= time:
                    artist, = plt.plot(time, ylevel, 'rx')
                    artist.set_label('HIV+')

                if self.transmission_occurred(partner, partnership):
                    self.plot_start_of_primary_infection(partner,
                                                          ylevel,
                                                          partnership)

        # If there are any pre-existing yticks, we need to append to them.
        yticks = gca.get_yticks()
        ytick_labels = [label.get_text() for label in gca.get_yticklabels()]
        if len(yticks) > 0:
            yticks = np.append(yticks, max_y)
            ytick_labels.append('CSW')
        else:
            # Ok, CSWs
            yticks = [max_y]
            ytick_labels = ['CSW']

        plt.yticks(yticks, ytick_labels)

    def _plot_casual(self, relmax):
        """Plot regular relationships.

        Plot Casual acts just below CSW.
        """
        gca = plt.gca()
        xlim = gca.get_xlim()
        ylim = gca.get_ylim()
        max_y = ylim[1] - 1

        casual_partnerships = [x for x in self.partnerships
                               if x.partnership_type == 'Casual']

        if len(casual_partnerships) == 0:
            # Do not bother if no casual partnerships.
            return

        # Go through each casual partnership.  If more than one
        # such partnership during a given timestep, we need to
        # stack them.
        for time in np.arange(xlim[1]):
            # Restrict to partnerships at this timestep.
            current_casuals = [x for x in casual_partnerships
                               if x.start == time]

            for j, partnership in enumerate(current_casuals):
                partner = partnership.partner_of(self)
                y_level = max_y - relmax['CSW'] - j

                artist, = plt.plot(time, y_level, 'g^')
                artist.set_label('Casual')
                if partner.hiv_positive and partner.time_of_infection <= time:
                    # overlay with red if HIV+
                    artist, = plt.plot(time, y_level, 'r^')
                    artist.set_label('HIV+')

                if self.transmission_occurred(partner, partnership):
                    self.plot_start_of_primary_infection(partner,
                                                          y_level,
                                                          partnership)

        # If there are any pre-existing yticks, we need to append to them.
        yticks = gca.get_yticks()
        ytick_labels = [label.get_text() for label in gca.get_yticklabels()]
        if len(yticks) > 0:
            yticks = np.append(yticks, max_y - relmax['CSW'])
            ytick_labels.append('Casual')
        else:
            # Ok, CSWs
            yticks = [max_y]
            ytick_labels = ['Casual']

        plt.yticks(yticks, ytick_labels)

    def _plot_regular(self, relmax):
        """Plot regular relationships.

        Instead of markers, these are spans.  Assumption is that the
        relationships are in order.
        """

        # Need a consistent way to disambiguate the end point of a
        # partnership.
        eps = 0.1

        gca = plt.gca()
        ylim = gca.get_ylim()
        max_y = ylim[1] - 1

        regular_partnerships = [x for x in self.partnerships
                                if x.partnership_type == 'Regular']

        if len(regular_partnerships) == 0:
            # Plot nothing if no regular partnerships.
            return

        max_num_casuals = relmax['Casual']
        line_lst = []
        for j in range(len(regular_partnerships)):
            regular_pship = regular_partnerships[j]
            #if regular_pship.person1.person_id == 268953 or regular_pship.person2.person_id == 268953:
            #    import pdb; pdb.set_trace()
            if j == 0:
                # Ok to plot in highest regular level
                y_level = max_y - relmax['CSW'] - max_num_casuals
            else:
                # check the previously plotted partnerships to see where we
                # can fit this.  We start out assuming that it must be at the
                # lowest level, but will check to see if we can push it
                # higher.  Push it as high as possible.
                prev_start = max(0, j - relmax['Regular'] - 1)
                bin_start = relmax['Steady'] + 1
                bin_end = max_y - relmax['CSW'] - relmax['Casual'] + 1
                bins = list(np.arange(bin_start, bin_end))

                # Go through the list of previous partnerships.  If the
                # previous partnership overlaps, then that y-level cannot be
                # chosen, so remove it from the bins.  When done, choose the
                # largest remaining bin.
                for k in range(prev_start, j):
                    prev = regular_partnerships[k]
                    if prev.start <= regular_pship.start and \
                       regular_pship.start < prev.start + prev.duration:
                        # Ok, they overlap, so remove this y-level from the
                        # bins.  We don't consider it a candidate anymore.
                        k_y_level = line_lst[k].get_ydata()[0]
                        bins.remove(k_y_level)
                y_level = max(bins)
            xcoords = [regular_pship.start,
                       regular_pship.start + regular_pship.duration]
            ycoords = [y_level, y_level]

            partner = regular_pship.partner_of(self)
            artist, = plt.plot(xcoords, ycoords, 'g-o')
            artist.set_label('Regular')
            line_lst.append(artist)

            # If either person is HIV+ before the end of the partnership,
            # make it red.
            if partner.hiv_positive:
                if partner.time_of_infection <= regular_pship.start:
                    # partner was infected before the relationship started
                    artist, = plt.plot(xcoords, ycoords, 'r-o')
                    artist.set_label('HIV+')

                end_of_partnership = regular_pship.start \
                    + regular_pship.duration + eps
                if partner.time_of_infection >= regular_pship.start and partner.time_of_infection <= end_of_partnership:
                    # partner became infected during the relationship.
                    xcoords = [partner.time_of_infection,
                               regular_pship.start + regular_pship.duration]
                    artist, = plt.plot(xcoords, ycoords, 'r-')
                    artist.set_label('HIV+')

                    # Make that last point red.
                    plt.plot(xcoords[1], ycoords[1], 'r-o')

                    plt.plot(partner.time_of_infection, ycoords[1], '-o', color='purple', alpha=0.5)

            if self.transmission_occurred(partner, regular_pship):
                self.plot_start_of_primary_infection(partner, y_level,
                                                     regular_pship)

        # If there are any pre-existing yticks, we need to append to them.
        gca = plt.gca()
        yticks = gca.get_yticks()
        ytick_labels = [label.get_text() for label in gca.get_yticklabels()]

        tick_regular = max_y - relmax['CSW'] - relmax['Casual']

        if len(yticks) > 0:
            yticks = np.append(yticks, tick_regular)
            ytick_labels.append('Regular')
        else:
            # Ok, only regulars up until this point.
            yticks = [tick_regular]
            ytick_labels = ['Regular']

        # if so, add to them.
        plt.yticks(yticks, ytick_labels)

    def _plot_steady(self):
        """Plot the steady relationships."""
        eps = 0.1

        # Steady relationships are easy.
        steady_partnerships = [x for x in self.partnerships
                               if x.partnership_type == 'Steady']

        if len(steady_partnerships) == 0:
            # Plot nothing if no steadys.
            return

        for pship in steady_partnerships:
            xcoords = [pship.start, pship.start + pship.duration]
            artist, = plt.plot(xcoords, [1, 1], 'g-s')
            artist.set_label('Steady')
            partner = pship.partner_of(self)

            # Overlay with red if infection has occurred.
            if partner.hiv_positive:
                if partner.time_of_infection <= pship.start:
                    # partner was infected before the relationship started
                    artist, = plt.plot(xcoords, [1, 1], 'r-s')
                    artist.set_label('HIV+')

                end_of_partnership = pship.start + pship.duration + eps
                if partner.time_of_infection >= pship.start and partner.time_of_infection <= end_of_partnership:
                    # partner became infected during the relationship.
                    end_of_partnership = pship.start + pship.duration
                    xcoords = [partner.time_of_infection, end_of_partnership]
                    artist, = plt.plot(xcoords, [1, 1], 'r-')
                    artist.set_label('HIV+')

                    # Make the last point red.
                    plt.plot(xcoords[1], [1], 'r-s')

                    # Make the time of infection an open purple circle.
                    plt.plot(partner.time_of_infection, [1], '-s', color='purple', alpha=0.5)
                    #plt.plot(partner.time_of_infection, [1], '-s', markerfacecolor=None, markeredgecolor='purple')

            if self.transmission_occurred(partner, pship):
                self.plot_start_of_primary_infection(partner, 1, pship)

        # Steadies are always plotted at y=1.
        plt.yticks([1], ['Steady'])

    def transmission_occurred(self, partner, partnership):
        """Determine if transmission occurred between self and partner."""
        # Both myself and my partner obviously must be HIV+
        epsilon = 0.1
        if self.hiv_positive and partner.hiv_positive:
            # If my partner was infected by me and the infection took place
            # during our partnership.
            #
            # Or if I was infected my partner.
            if (((partner.infected_by == self.person_id) and
                 (partnership.start <= partner.time_of_infection) and
                 (partner.time_of_infection < (partnership.start +
                                               partnership.duration +
                                               epsilon))) or
                ((self.infected_by == partner.person_id) and
                 (partnership.start <= self.time_of_infection) and
                 (self.time_of_infection < (partnership.start +
                                            partnership.duration +
                                            epsilon)))):

                return True

        # Otherwise no, transmission did not occur.
        return False

    def plot_start_of_primary_infection(self, partner, ycoord, prtnrshp):
        """Mark start of primary infection transmission with purple dot."""

        if prtnrshp.partnership_type == 'CSW':
            marker = 'x'
            label = 'CSW transmission'
            markersize = 10 
        elif prtnrshp.partnership_type == 'Casual':
            marker = '^'
            label = 'Casual transmission'
            markersize = 8
        elif prtnrshp.partnership_type == 'Regular':
            marker = 'o'
            label = 'Regular transmission'
            markersize = 8
        elif prtnrshp.partnership_type == 'Steady':
            marker = 's'
            label = 'Steady transmission'
            markersize = 8

        # If I am HIV+ and I infected my partner, then denote that with
        # a # purple marker.
        if partner.infected_by == self.person_id:
            plt.plot(partner.time_of_infection, ycoord,
                     color='purple',
                     marker=marker,
                     label=label,
                     markersize=markersize)

        # If my partner is HIV+ and infected me, then denote that with a
        # red marker.
        if self.infected_by == partner.person_id:
            plt.plot(self.time_of_infection, ycoord,
                     color='red',
                     marker=marker,
                     label=label,
                     markersize=8)


class Male(Person):
    """Class for a male individual.

    Attributes
    ----------
    gender : str
        Either 'MALE' or 'FEMALE'
    csw : bool
        True if currently a CSW.
    ever_csw : bool
        True if has ever been a CSW (maybe they retired).
    single : bool
        True if currently not in a relationship.
    age : int
        Current age in months.
    risk : str
        Either 'LOW' or 'HIGH'
    traced : bool
        True if this person is being traced.  An individual might show up in
        the simulation if they are not being traced, but was a partner of
        someone who IS being traced.
    """
    def __init__(self, person_id=None, csw=False, single=False, age=-1,
                 risk='LOW', traced=False):
        """Create a new male agent."""
        Person.__init__(self, person_id)
        self.gender = 'MALE'
        self.csw = csw == 'CSW'
        self.ever_csw = self.csw
        self.single = single == 'SINGLE'
        self.age = int(age)
        self.risk = risk
        self.traced = traced


class Female(Person):
    """Class for a female individual.

    Attributes
    ----------
    gender : str
        Either 'MALE' or 'FEMALE'
    csw : bool
        True if currently a CSW.
    ever_csw : bool
        True if has ever been a CSW (maybe they retired).
    single : bool
        True if currently not in a relationship.
    age : int
        Current age in months.
    risk : str
        Either 'LOW' or 'HIGH'
    traced : bool
        True if this person is being traced.  An individual might show up in
        the simulation if they are not being traced, but was a partner of
        someone who IS being traced.
    """
    def __init__(self, person_id=None, csw=False, single=False, age=-1,
                 risk='LOW', traced=False):
        """Create a new female agent."""
        Person.__init__(self, person_id)
        self.gender = 'FEMALE'
        self.csw = csw == 'CSW'
        self.ever_csw = self.csw
        self.single = single == 'SINGLE'
        self.age = int(age)
        self.risk = risk
        self.traced = traced


class Population:
    """Defines the population in the simulation.

    Attributes
    ----------
    file:  string
        trace file from transmission model output
    person:  dictionary
        dictionary-like collection of Person(s) where the key is the
        person's numeric ID.
    partnerships:  list
        List of partnerships at each timestep
    current_partnerships : list
        List of partnerships at this timestep only.
    """
    def __init__(self, trace_file):
        self.file = trace_file
        self.person = {}
        self.partnerships = []

        # This keeps track of all the partnerships in a particular
        # timestep.
        self.current_partnerships = []

        # This marks a partnership attempt by a male.  It may or may not be
        # successful.  If it is, we follow up with a partnership.  Otherwise
        # we go on.
        #
        # + Male 123 attempts to form 1 Steady partnerships:
        _ = re.compile(r"""\+\sMale\s(?P<male_id>\d+)\s+
                           attempts\sto\sform\s
                           (?P<numPartnerships>\d+)\s
                           (?P<reltype>(Steady|Casual|Regular|CSW))\s+
                           partnerships:""", re.VERBOSE)
        self.male_partnership_attempt_regex = _

        # This marks a female being chosen.  It is ALWAYS followed
        # up with a partnership.
        #
        # + Female 123 is chosen for a Steady partnership:
        #
        # We only need to match the ID here.
        _ = re.compile(r"""\+\sFemale\s(?P<id>\d+)\s+""", re.VERBOSE)
        self.female_chosen_partnership_regex = _

        # Repeat partnerships are not allowed.  Skip them when encountered.
        _ = re.compile(r"""\s+\+x\sMale\s(?P<male_id>\d+)\s+
                           attempted\srepeat\spartnership\swith
                           (?P<female_id>)""", re.VERBOSE)
        self.repeat_partnership_regex = _

        # Denotes a successful partnership being formed.
        #
        # + Male 123 (NON_CSW:SINGLE:MALE age 23) forms Steady with female 234
        #     (NON_CSW:SINGLE:FEMALE age 22, 1 marbles, LOW risk) of duration
        #     11
        _ = re.compile(r"""\s+\+\sMale\s(?P<male_id>\d+)\s+
                           \(
                           (?P<initiator_csw_status>\S+):
                           (?P<initiator_single>\S+):
                           (?P<initiator_gender>\S+)\s+
                           age\s(?P<initiator_age>\d+)
                           \)\s+
                           forms\s+
                           (?P<reltype>(Steady|Casual|Regular|CSW))\s+
                           with\sfemale\s(?P<female_id>\d+)\s+
                           \(
                           (?P<partner_csw_status>\S+):
                           (?P<partner_single>\S+):
                           (?P<partner_gender>\S+)\s+
                           age\s(?P<partner_age>\d+),\s\d+\smarbles,\s+
                           (?P<partner_risk>(HIGH|LOW))\srisk
                           \)
                           (\s+of\sduration\s(?P<duration>\d+)){0,1}""",
                       re.VERBOSE)
        self.partnership_formed_regex = _

    def __str__(self):
        nmales = len([x for x in self.person.values() if x.gender == 'MALE'])
        nfemales = len(self.person) - nmales
        msg = "The population has %d males, %d females." % (nmales, nfemales)
        return msg

    def update_partnership_lists(self, timestep):
        """Update partnership lists.

        Parameters
        ----------
        timestep : int
            Current timestep.

        Add the list of partnerships for the specified timestep to the list
        of all partnerships.  Prune the list of current partnerships if they
        are not really current.
        """
        self.partnerships.append(self.current_partnerships)

        # Remove any partnerships from the list that do not carry over.
        lst = [x for x in self.current_partnerships
               if timestep <= x.start + x.duration]
        self.current_partnerships = lst

    def populate(self):
        """Create the initial population."""
        # Sweep through the file looking for consecutive lines like
        #
        # Tracing the following patient:
        # Male    ID: 0   (NON_CSW:SINGLE:MALE)   Age(mos.): 53   \
        #        CD4: -1 HVL: -1 Risk: LOW       Marbles: 1
        _ = re.compile(r"""(?P<gender>(Male|Female))\s+  # gender
                           ID:\s(?P<id>\d+)\s+           # match the ID
                           \(                            #
                           (?P<csw_status>\S+):          # CSW or not
                           (?P<single>\S+):              # relationship status
                           (?P<gender2>\S+)              # unneeded
                           \)\s+                         #
                           Age\(mos.\):\s(?P<age>\d+)\s+ # Age
                           CD4:\s+-1\s+                  # CD4 -1 to start
                           HVL:\s+-1\s+                  # HVL -1 to start
                           Risk:\s+(?P<risk>LOW|HIGH)\s+ # Risk
                           Marbles:\s+(?P<marbles>\d+)   # Risk""", re.VERBOSE)
        regex = _

        with open(self.file, 'r') as fptr:
            try:
                while True:
                    line = next(fptr).rstrip()

                    if line.startswith('Tracing the following patient:'):
                        line = next(fptr).rstrip()
                        match = regex.match(line)
                        if match is not None:
                            person_id = int(match.group('id'))
                            if match.group('gender') == 'Male':
                                person = Male(person_id=person_id,
                                              csw=match.group('csw_status'),
                                              single=match.group('single'),
                                              age=int(match.group('age')),
                                              traced=True,
                                              risk=match.group('risk'))
                            else:
                                person = Female(person_id=person_id,
                                                csw=match.group('csw_status'),
                                                single=match.group('single'),
                                                age=int(match.group('age')),
                                                traced=True,
                                                risk=match.group('risk'))
                            self.person[person.person_id] = person

            except StopIteration:
                pass

    def add_to_population(self, line):
        """This individual is to be traced.  Add to the population."""
        # Sweep through the file looking for consecutive lines like
        #
        # Tracing the following patient:
        # Male    ID: 0   (NON_CSW:SINGLE:MALE)   Age(mos.): 53   \
        #        CD4: -1 HVL: -1 Risk: LOW       Marbles: 1
        _ = re.compile(r"""(?P<gender>(Male|Female))\s+  # gender
                           ID:\s(?P<id>\d+)\s+           # match the ID
                           \(                            #
                           (?P<csw_status>\S+):          # CSW or not
                           (?P<single>\S+):              # relationship status
                           (?P<gender2>\S+)              # unneeded
                           \)\s+                         #
                           Age\(mos.\):\s(?P<age>\d+)\s+ # Age
                           CD4:\s+-1\s+                  # CD4 -1 to start
                           HVL:\s+-1\s+                  # HVL -1 to start
                           Risk:\s+(?P<risk>LOW|HIGH)\s+ # Risk
                           Marbles:\s+(?P<marbles>\d+)   # Risk""", re.VERBOSE)
        regex = _

        match = regex.match(line)
        if match is not None:
            person_id = int(match.group('id'))
            if match.group('gender') == 'Male':
                person = Male(person_id=person_id,
                              csw=match.group('csw_status'),
                              single=match.group('single'),
                              age=int(match.group('age')),
                              traced=True,
                              risk=match.group('risk'))
            else:
                person = Female(person_id=person_id,
                                csw=match.group('csw_status'),
                                single=match.group('single'),
                                age=int(match.group('age')),
                                traced=True,
                                risk=match.group('risk'))
            self.person[person.person_id] = person


    def process_new_partnership(self, match, start):
        """Process just this partnership at this time.

        The line should look something like:

        + Male 587 (NON_CSW:SINGLE:MALE age 21) forms Steady with female 785
            (NON_CSW:SINGLE:FEMALE age 20, 1 marbles, LOW risk) of duration 3

        Parameters
        ----------
        match : re.MatchObject
            Result of regular expression being run against line of text of
            partnership data.
        start : int
            Timestep of start of partnership.
        """
        m_id = int(match.group('male_id'))
        f_id = int(match.group('female_id'))

        try:
            self.person[m_id]
        except KeyError:
            # This male wasn't being traced.  Nevertheless we have
            # to add him in.  We don't know what his risk is, so it
            # defaults.
            male = Male(person_id=int(match.group('male_id')),
                        csw=match.group('initiator_csw_status'),
                        single=match.group('initiator_single'),
                        age=int(match.group('initiator_age')) * 12)
            self.person[m_id] = male

        try:
            self.person[f_id]
        except KeyError:
            # This female wasn't being traced.  Nevertheless we have
            # to add her in.
            female = Female(person_id=int(match.group('female_id')),
                            csw=match.group('partner_csw_status'),
                            single=match.group('partner_single'),
                            age=int(match.group('partner_age')) * 12,
                            risk=match.group('partner_risk'))
            self.person[f_id] = female

        male = self.person[m_id]
        female = self.person[f_id]

        # If neither person is being traced, then something seems amiss.
        # Still record it, but issue a warning.
        if not male.traced and not female.traced:
            msg = "Unusual partnership attempt in trace file, "
            msg += "neither {0} nor {1} are actively being traced."
            msg = msg.format(m_id, f_id)
            warnings.warn(msg)

        reltype = match.group('reltype')

        # The "duration" part might not be present.  If not and it is a CSW,
        # then the duration should be zero.
        if ((('duration' in match.groupdict()) and
             (match.group('duration') is not None))):
            duration = int(match.group('duration'))
        else:
            duration = 0

        pship = Partnership(person1=male, person2=female,
                            partnership_type=reltype,
                            duration=duration, start=start)
        self.current_partnerships.append(pship)


class TraceSimulation:
    """Simulates transmission model simulation by parsing the trace file.

    Attributes
    ----------
    act_regex : regular expression
        Matches trace file lines specifying sex acts.    
    csw_retirement_regex : regular expression
        Matches trace file lines specifying that a particular CSW is retiring.
    death_regex : regular expression
        Matches trace file lines specifying who has died.
    end_partnership_regex : regular expression
        Matches trace file lines specifying who has broken up with who.
    incident_hiv_regex : regular expression
        Matches trace file lines specifying who now has an incident case of
        HIV.
    infected_regex : regular expression
        Matches trace file lines specifying who has infected whom with HIV.
    prevalence_delay : int
        When HIV is introduced.  Usually around timestep 600.
    prevalent_hiv_regex : regular expression
        Matches trace file lines specifying who has been chosen for a prevalent
        case of HIV.
    rerolls_risk_regex : regular expression
        Matches trace file lines specifying whose risk status will change.
    sexual_debut_regex : regular expression
        Matches trace file lines specifying someone's sexual debut.
    timestep_regex : regular expression
        Matches trace file lines specifying that the simulation timestep has
        changed.
    trace_delay : int
        Timestep at which to start tracing individuals.  Before 3.32j, this was
        timestep 0, but following 3.32j it is configurable.
    transition_to_csw_regex : regular expression
        Matches trace file lines specifying that a person transitioned to being
        a CSW.
    """
    def __init__(self, xmlfile):

        # Setup defaults to be filled in by the _setup_regular_expressions
        # method.
        self.act_regex = None
        self.csw_retirement_regex = None
        self.death_regex = None
        self.end_partnership_regex = None
        self.incident_hiv_regex = None
        self.infected_regex = None
        self.rerolls_risk_regex = None
        self.prevalent_hiv_regex = None
        self.sexual_debut_regex = None
        self.timestep_regex = None
        self.transition_to_csw_regex = None
        self._setup_regular_expressions()

        # Set up some defaults.  These will be overwritten when the xml control
        # file is parsed.
        self.prevalence_delay = -1
        self.trace_delay = 0
        self.length_of_simulation = -1

        try:
            self.parse_xml_control_file(xmlfile)
        except ET.ParseError:
            msg = "XML file not provided.  "
            msg += "HIV stage information will not be available"
            warnings.warn(msg)
            self.trace_file = xmlfile

            self.male_transm_coefficients = None
            self.male_primary_stage_hvl = None
            self.male_late_stage_hvl = None

            self.female_transm_coefficients = None
            self.female_primary_stage_hvl = None
            self.female_late_stage_hvl = None

        self.population = Population(self.trace_file)

        self.timestep = 0

        self._run()

    def parse_xml_control_file(self, xmlfile):
        """Parse the xml control file.

        We are after the following bits of information.

            Male transmission coefficients.
            Female transmission coefficients.
            Name of singleperson trace file.
            Prevalence delay.

        """
        tree = ET.parse(xmlfile)
        root = tree.getroot()
        elts = root.findall('population/entityTypes/baseEntities/baseEntity')
        if len(elts) != 2:
            msg = "Did not find both male and female base entities."
            raise RuntimeError(msg)

        self.parse_base_entity(elts[0])
        self.parse_base_entity(elts[1])

        # Get the trace file.  First, is it even being traced?
        elts = root.findall('writeTrace/singleperson')
        if elts[0].text != '1':
            raise RuntimeError("No trace file created.")

        dirname = os.path.dirname(xmlfile)
        basename = os.path.basename(xmlfile)
        model_run_id = os.path.splitext(basename)[0]

        # Get the extension.
        elts = root.findall('extensionNames/singleperson')
        extension = elts[0].text

        singleperson_basename = model_run_id + '-' + extension
        self.trace_file = os.path.join(dirname,
                                       'results',
                                       singleperson_basename)

        # Get the length of the simulation.
        elts = root.findall('timeLimitMth')
        if len(elts) != 1:
            raise RuntimeError("No simulation length found.")
        self.length_of_simulation = int(elts[0].text)

        # Get the time of delay.
        elts = root.findall('population/initialState/delay')
        if len(elts) != 1:
            raise RuntimeError("No delay found.")
        self.prevalence_delay = int(elts[0].text)

        # Get the time of trace delay (if it exists)
        # This was introduced in 3.32j
        elts = root.findall('monthTraceNewborns')
        if len(elts) > 0:
            self.trace_delay = int(elts[0].text)

    def parse_base_entity(self, elt):
        """Parse a base entity for transmission coefficient information.

        <health>
          <circumcisionProtectEfficacy>0.56</circumcisionProtectEfficacy>
          <condomProtectEfficacy>0.8</condomProtectEfficacy>
          <transmissionCoefficients>
            <valsByHVL>
                0.0001 0.0001 0.0012 0.0012 0.0014 0.0023 0.0023
            </valsByHVL>
            <primary>0.00819</primary>
            <lateStage>0.00355</lateStage>
          </transmissionCoefficients>
        </health>

        Parameters
        ----------
        elt : ElementTree Element
            XML entity containing either male or female information.
        """
        elts = elt.findall('health/transmissionCoefficients/valsByHVL')
        val_lst = elts[0].text.split()
        transmission_coefficients = [float(x) for x in val_lst]

        elts = elt.findall('health/transmissionCoefficients/primary')
        primary_stage_hvl = float(elts[0].text)

        elts = elt.findall('health/transmissionCoefficients/lateStage')
        late_stage_hvl = float(elts[0].text)

        gender_lst = elt.findall('type')
        gender = gender_lst[0].text
        if gender == 'Male':
            self.male_transm_coefficients = transmission_coefficients
            self.male_primary_stage_hvl = primary_stage_hvl
            self.male_late_stage_hvl = late_stage_hvl
        else:
            self.female_transm_coefficients = transmission_coefficients
            self.female_primary_stage_hvl = primary_stage_hvl
            self.female_late_stage_hvl = late_stage_hvl

    def _plot_background(self, verts, facecolor, label):
        """Plot a polygon patch with the given color."""
        ax1 = plt.gca()
        codes = [Path.MOVETO,
                 Path.LINETO,
                 Path.LINETO,
                 Path.LINETO,
                 Path.CLOSEPOLY,
                 ]
        path = Path(verts, codes)
        patch = patches.PathPatch(path,
                                  facecolor=facecolor,
                                  label=label,
                                  alpha=0.2)
        ax1.add_patch(patch)

    def _plot_person_uninfected_time(self, person):
        """Plot green background for uninfected time.

        Parameters
        ----------
        person : Person instance
            Individual being traced in the plot.
        """

        axcoord1 = plt.gca()
        ylim = axcoord1.get_ylim()
        ycoord0 = ylim[0]
        ycoord1 = ylim[1]

        # Add the green background for uninfected time.
        xcoord0 = 0
        xcoord1 = person.time_of_infection
        verts = [(xcoord0, ycoord0),      # left, bottom
                 (xcoord0, ycoord1),      # left, top
                 (xcoord1, ycoord1),      # right, top
                 (xcoord1, ycoord0),      # right, bottom
                 (xcoord0, ycoord0)]

        self._plot_background(verts, 'green', 'uninfected stage')

    def _plot_person_primary_stage(self, person):
        """Plot a purple background for the primary stage infection.

        Parameters
        ----------
        person : Person instance
            Individual being traced in the plot.
        """

        ax1 = plt.gca()
        ylim = ax1.get_ylim()
        ycoord0 = ylim[0]
        ycoord1 = ylim[1]

        xcoord0 = person.time_of_infection
        xcoord1 = xcoord0 + 3

        verts = [(xcoord0, ycoord0),      # left, bottom
                 (xcoord0, ycoord1),      # left, top
                 (xcoord1, ycoord1),      # right, top
                 (xcoord1, ycoord0),      # right, bottom
                 (xcoord0, ycoord0)]

        self._plot_background(verts, 'purple', 'primary stage')

    def _plot_person_chronic_stage(self, person):
        """Plot a red background for the chronic stage infection.

        Parameters
        ----------
        person : Person instance
            Individual being traced in the plot.
        """

        ax1 = plt.gca()
        ylim = ax1.get_ylim()
        ycoord0 = ylim[0]
        ycoord1 = ylim[1]

        xcoord0 = person.time_of_infection + 3

        chronic_times = [x[0] for x in person.transmission_coefficient
                         if x[1] < self.male_primary_stage_hvl
                         and x[1] < self.male_late_stage_hvl]
        late_stage_times = [x[0] for x in person.transmission_coefficient
                            if x[1] == self.male_late_stage_hvl]

        if len(chronic_times) == 0 and len(late_stage_times) == 0:
            # Did not detect chronic or late stage.
            if person.time_of_death == -1:
                # person did not die.  End of chronic is end of simulation.
                xcoord1 = self.length_of_simulation
            else:
                # End of chronic is time of death.
                xcoord1 = person.time_of_death

        elif len(chronic_times) == 0:
            # did not detect chronic stage, but did detect late stage.
            # End of chronic is the start of late stage.
            xcoord1 = late_stage_times[0]

        elif len(late_stage_times) == 0:
            # did not detect late stage, but did detect chronic stage.
            # If the person died, that is the end of the chronic stage.
            if person.time_of_death > -1:
                xcoord1 = person.time_of_death
            else:
                xcoord1 = self.length_of_simulation

        else:
            # Detected both chronic and late stage.
            # End of chronic is start of late stage.
            xcoord1 = late_stage_times[0]

        verts = [(xcoord0, ycoord0),      # left, bottom
                 (xcoord0, ycoord1),      # left, top
                 (xcoord1, ycoord1),      # right, top
                 (xcoord1, ycoord0),      # right, bottom
                 (xcoord0, ycoord0)]

        self._plot_background(verts, 'red', 'chronic stage')

    def _plot_person_late_stage(self, person):
        """Plot a red background for the late stage infection.

        Parameters
        ----------
        person : Person instance
            Individual being traced in the plot.
        """

        ax1 = plt.gca()
        ylim = ax1.get_ylim()
        ycoord0 = ylim[0]
        ycoord1 = ylim[1]

        late_stage_times = [x[0] for x in person.transmission_coefficient
                            if x[1] == self.male_late_stage_hvl]

        if len(late_stage_times) == 0:
            warnings.warn("No late stage detected???", UserWarning)
            return

        xcoord0 = late_stage_times[0]
        if person.time_of_death > -1:
            xcoord1 = person.time_of_death
        else:
            xcoord1 = self.length_of_simulation

        verts = [(xcoord0, ycoord0),      # left, bottom
                 (xcoord0, ycoord1),      # left, top
                 (xcoord1, ycoord1),      # right, top
                 (xcoord1, ycoord0),      # right, bottom
                 (xcoord0, ycoord0)]

        self._plot_background(verts, 'yellow', 'late stage')

    def plot_person(self, pid, plot, xlim):
        """
        Plots an individual person.  Most of the work is handled by the Person
        structure, but transmission coefficient handling must be done here.
        """
        person = self.population.person[pid]
        ax1 = person.plot(plot.figure)

        # On windows, ytick labels vanish on the code that follows, so
        # let's save them and reapply at the end.
        yticks = ax1.get_yticks()
        ytick_labels = [label.get_text() for label in ax1.get_yticklabels()]

        if xlim is not None:
            # Probably called by the GUI.
            ax1.set_xlim(xlim)

        if person.hiv_positive:
            self._plot_person_late_stage(person)
            self._plot_person_chronic_stage(person)
            self._plot_person_primary_stage(person)
            self._plot_person_uninfected_time(person)

        # Create a second axis that is twinned to the current Y axis.
        # Set the age of the person to the 2nd axis xticks.
        xticks1 = ax1.get_xticks()
        ax2 = ax1.twiny()
        ax2.set_xticks(xticks1)

        # Display the 2nd axis in years, not months.
        if self.trace_delay == 0:
            age_xlabels = ["{0:.1f}".format(float(x + person.age) / 12) for x in xticks1]
        else:
            age_xlabels = []
            for x in xticks1:
                if x - self.trace_delay < 0:
                    age_xlabels.append('')
                else:
                    age_xlabels.append("{0:.1f}".format(float(x - self.trace_delay) / 12))

        model_xticks = ax1.get_xticks()
        model_xtick_labels = []
        for xtick in model_xticks:
            model_xtick_labels.append(str(int(xtick)))

        ax2.set_xlim(ax1.get_xlim())

        ax1.set_label('Main axis')
        ax2.set_label('Person age')
        ax1.set_yticks(yticks)
        ax1.set_yticklabels(ytick_labels)

        # Switch the labels to have model year on top, age on bottom.
        ax1.set_xticklabels(age_xlabels)
        ax2.set_xticklabels(model_xtick_labels)

##        if visible:
        plot.figure.canvas.draw()

    def _setup_regular_expressions(self):
        """Define regular expressions used during trace file parsing.
        """
        # This matches a timestep line
        _ = re.compile(r"""\*\*\sTime\s(?P<timestep>\d+)""", re.VERBOSE)
        self.timestep_regex = _

        # Male, female sexual debut.
        _ = re.compile(r"""\s%\s(Male|Female)\s
                           (?P<id>\d+)\s
                           becomes\ssexually\sactive""", re.VERBOSE)
        self.sexual_debut_regex = _

        # Transition to CSW.
        _ = re.compile(r"""\s%\s(Male|Female)\s
                           (?P<id>\d+)\s
                           becomes\sCSW""", re.VERBOSE)
        self.transition_to_csw_regex = _

        # person rerolls risk
        _ = re.compile(r"""\s%\s(?P<gender>(Male|Female))\s
                           (?P<id>\d+)\s
                           rerolls\sas\s
                           (?P<risk>(High|Low))\s
                           risk""", re.VERBOSE)
        self.rerolls_risk_regex = _

        # This line is hit after it is announced that someone died.
        #
        # Male    ID: 884 (NON_CSW:NON_SINGLE:MALE)
        #     Age(mos.): 895  CD4: -1 HVL: -1 Risk: LOW       Marbles: 1
        _ = re.compile(r"""(Male|Female)\s+ID:\s+
                           (?P<id>\d+)\s+
                           \(
                           (?P<csw_status>\S+):
                           (?P<single_status>\S+):
                           (?P<gender>\S+)
                           \)\s+
                           Age\(mos.\):\s+(?P<age>\d+)\s+
                           CD4:\s(?P<cd4_posneg>-?)
                           (?P<cd4>\d+)(?P<cd4frac>(\.\d+){0,1})\s+
                           HVL:\s(?P<hvl_posneg>-?)(?P<hvl>\d+)\s+
                           Risk:\s+(?P<risk>(HIGH|LOW))\s+
                           Marbles:\s\d+""", re.VERBOSE)
        self.death_regex = _

        _ = re.compile(r"""@\s(?P<gender>(Male|Female))\s
                           (?P<id>\d+)\s
                           has\san\sincident\scase\sof\sHIV!""", re.VERBOSE)
        self.incident_hiv_regex = _

        _ = re.compile(r"""\s\%\s(?P<gender>(Male|Female))\s+
                           (?P<id>\d+)\s+
                           quits\sbeing\sCSW""", re.VERBOSE)
        self.csw_retirement_regex = _

        # '@ Female 217 has a prevalent case of HIV!'
        _ = re.compile(r"""@\s(?P<gender>(Male|Female))\s
                           (?P<id>\d+)\s
                           has\sa\sprevalent\scase\sof\sHIV!""", re.VERBOSE)
        self.prevalent_hiv_regex = _

        # Sex acts, e.g.
        #
        # '# Male 419 engages in 6 acts with his Steady 641'
        _ = re.compile(r"""(?P<initiator_gender>(Male|Female))\s
                           (?P<initiator_id>\d+)\s
                           engages\sin\s
                           (?P<num_acts>\d+)
                           \sacts\swith\s(his|her)\s
                           (?P<reltype>(Steady|Casual|Regular|CSW))\s
                           (?P<partner_id>\d+)""", re.VERBOSE)
        self.act_regex = _

        # "- Male 433 ends Steady partnership with 559"
        # "- Female 949 ends Steady partnership with 905"
        # "- Female 218 ends Steady partnership with 658 -> 218 has died"
        _ = re.compile(r"""-\s(?P<initiator_gender>(Male|Female))\s
                           (?P<initiator_id>\d+)\s
                           ends\s
                           (?P<reltype>(Steady|Casual|Regular|CSW))\s+
                           partnership\swith\s
                           (?P<partner_id>\d+)
                           (\s+->(?P<has_died>\s\d+)\shas\sdied){0,1}
                           """, re.VERBOSE)
        self.end_partnership_regex = _

        # " !!# 1 infected 2!"
        _ = re.compile(r"""\s!!\#\s
                           (?P<infector>\d+)\s+
                           infected\s+
                           (?P<partner>\d+)!""", re.VERBOSE)
        self.infected_regex = _

    def process_end_partnership(self, line):
        """Process the end of a partnership.

        This will be denoted by a line reading something like:

            "- Male 433 ends Steady partnership with 559"
            "- Female 949 ends Steady partnership with 905"

        Args:
            line:  string
                Line of text from the trace file.
        """
        match = self.end_partnership_regex.match(line)
        if match is None:
            raise ParseRuntimeError(line)

        initiator_id = int(match.group('initiator_id'))
        partner_id = int(match.group('partner_id'))
        if initiator_id not in self.population.person and partner_id not in self.population.person:
            # Neither individual is being traced, so we are tracking a 
            # partnership we do not care about.
            return

        self.verify_partner(match)
        initiator = self.population.person[initiator_id]
        partner = self.population.person[partner_id]

        # Find the partnership.  There had better just be one of them!
        parts = [x for x in initiator.partnerships
                 if x.partner_of(initiator).person_id == partner.person_id
                 and x.alive is True]
        if len(parts) != 1:
            raise RuntimeError(line)
        parts[0].alive = False

        # Always fix the duration here because we cannot always be certain of
        # when the partnership started (if tracing after t=0).  Also, if a
        # partner dies, the initial duration would be incorrect.
        parts[0].duration = self.timestep - parts[0].start - 1

        if ((('has_died' in match.groupdict()) and
             (match.group('has_died') is not None))):
            pid = int(match.group('has_died'))
            person_who_died = self.population.person[pid]
            person_who_died.time_of_death = self.timestep

    def verify_partner(self, match):
        """Verify that the partner is in the population.

        We are here because some sort of partnership situation was encountered.
        We need to make sure that the partner exists.  If not, put them in.
        """
        initiator_id = int(match.group('initiator_id'))
        partner_id = int(match.group('partner_id'))

        initiator = self.population.person[initiator_id]
        if partner_id in self.population.person:
            partner = self.population.person[partner_id]
        else:
            # Must add in this person.
            if match.group('initiator_gender').lower() == 'male':
                # initiator was male, not the partner.
                partner = Female(person_id=partner_id, traced=False)
            else:
                partner = Male(person_id=partner_id, traced=False)
            self.population.person[partner_id] = partner

        # Check if the partnership exists.
        parts = [x for x in initiator.partnerships
                 if x.partner_of(initiator).person_id == partner.person_id
                 and x.alive is True]
        if len(parts) == 1:
            # Partnership exists, so we are done.
            return

        # Add the partnership into the mix.  Best we can do is
        # assume it started when tracing started.
        reltype = match.group('reltype')
        if reltype == 'CSW' or reltype == 'Casual':
            duration = 1
        else:
            # Have no idea how long it is to last.
            duration = self.length_of_simulation - self.prevalence_delay
        pship = Partnership(person1=initiator, person2=partner,
                            partnership_type=reltype,
                            duration=duration,
                            start=self.prevalence_delay)
        self.population.current_partnerships.append(pship)

    def process_acts(self, line):
        """Process trace lines denoting sex acts.

        # '# Male 419 engages in 6 acts with his Steady 641'
        
        But having the octothorpe in the line will cause troubles
        # with re.VERBOSE, so we skip over it.
        """

        match = self.act_regex.match(line[2:])
        if match is None:
            raise RuntimeError(line)

        initiator_id = int(match.group('initiator_id'))
        partner_id = int(match.group('partner_id'))
        if initiator_id not in self.population.person and partner_id not in self.population.person:
            # Neither individual is being traced, so we are tracking a 
            # partnership we do not care about.
            return

        self.verify_partner(match)
        initiator = self.population.person[initiator_id]
        partner = self.population.person[int(match.group('partner_id'))]

        num_acts = int(match.group('num_acts'))

        # Ok, find the partnership.
        parts = [x for x in initiator.partnerships
                 if x.partner_of(initiator).person_id == partner.person_id
                 and self.timestep <= (x.start + x.duration)
                 and x.alive is True]
        if len(parts) > 1:
            for part in parts:
                print(part)
        if len(parts) != 1:
            raise RuntimeError(line)
        parts[0].num_acts += num_acts

    def process_death(self, line):
        """Process the death of an individual.

        The line of text looks something like

        Male    ID: 884 (NON_CSW:NON_SINGLE:MALE)
            Age(mos.): 895  CD4: -1 HVL: -1 Risk: LOW       Marbles: 1

        Parameters
        ----------
        line : str
            Line of text from the trace file.  The line is preceded by the
            line

                ">> Today we mourn:"

            The identifying line should like something like as follows:

                "Female   ID: 218   (NON_CSW:NON_SINGLE:FEMALE)
                 Age(mos.): 369 CD4: 405.146    HVL: 6    Risk: HIGH
                 Marbles: 1"

            although the white space may be different.
        """
        match = self.death_regex.match(line)
        if match is None:
            raise RuntimeError(line)

        pid = int(match.group('id'))

        if pid not in self.population.person:
            # This person must be untraced.  Add them in just for the sake of
            # completeness.
            if match.group('gender') == 'MALE':
                person = Male(person_id=pid,
                              csw=match.group('csw_status'),
                              single=match.group('single_status'),
                              age=int(match.group('age')),
                              risk=match.group('risk'))
            else:
                person = Female(person_id=pid,
                                csw=match.group('csw_status'),
                                single=match.group('single_status'),
                                age=int(match.group('age')),
                                risk=match.group('risk'))
            self.population.person[pid] = person

        self.population.person[pid].time_of_death = self.timestep
        self.population.person[pid].alive = False

    def new_partnership(self, line):
        """Process a new partnership.

        The line should look something like

        + Male 587 (NON_CSW:SINGLE:MALE age 21) forms Steady with female 785
            (NON_CSW:SINGLE:FEMALE age 20, 1 marbles, LOW risk) of duration
            3
        """
        match = self.population.partnership_formed_regex.match(line)
        if match is not None:
            self.population.process_new_partnership(match, self.timestep)
        else:
            raise ParseRuntimeError(line)

    def process_incident_hiv(self, line):
        """Process an incident HIV case.

        The line of text signalling this looks something like:

            '@ Male 658 has an incident case of HIV!'

        Args:
            line:  string
                Line of text from the trace file.
        """
        match = self.incident_hiv_regex.match(line)
        pid = int(match.group('id'))
        self.population.person[pid].set_hiv_infection(self.timestep)

    def process_prevalent_hiv(self, line):
        """Process a prevalent case of HIV.

        The line of text signalling this looks something like:

            '@ Female 217 has a prevalent case of HIV!'

        Parameters
        ----------
        line:  string
            Line of text from the trace file.
        """
        match = self.prevalent_hiv_regex.match(line)
        pid = int(match.group('id'))
        self.population.person[pid].set_hiv_infection(self.timestep)

    def process_transition_to_csw(self, line):
        """After sexual debut, a person can transition to CSW.

        The line of text signalling this looks something like:

            ' % Female 2736783 becomes CSW'

        Parameters
        ----------
        line:  string
            Line of text from the trace file.
        """
        match = self.transition_to_csw_regex.match(line)
        pid = int(match.group('id'))
        if pid not in self.population.person:
            # If this person is not in the population yet, ignore this.
            return

        self.population.person[pid].csw = True
        self.population.person[pid].ever_csw = True

    def reroll_risk(self, line):
        """Reset a person's risk level.

        Parameters
        ----------
        line:  string
            Line of text from the trace file.
        """
        match = self.rerolls_risk_regex.match(line)
        pid = int(match.group('id'))

        if pid not in self.population.person:
            # If this person is not in the population yet, ignore this.
            return

        # Force risk to always be upper case.
        self.population.person[pid].risk = match.group('risk').upper()

    def process_transmission_act(self, line):
        """Process a possible transmission.

        There are 6 possibilities here.

        1.  ' !Transmission coefficient from 217 to 127 is 0.036;'
        2.  ' !A condom was NOT used (efficacy 0.8);'
        3.  ' !127 is circumcised (efficacy 0.56);'
        4.  ' !Total FOI = 0.01584'
        5.  ' !# 217 exposed but did not infect 127!'
        6.  ' !# 217 infected 159!'

        For now, we only care about cases 1 and 4 and 6.
        """
        if line.startswith(' !Transmission coefficient'):
            regex = re.compile(r"""\s!Transmission\scoefficient\sfrom\s
                                   (?P<initiator>\d+)\s
                                   to\s
                                   (?P<partner>\d+)\s
                                   is\s(?P<transmission_coeff>0\.\d+)""",
                               re.VERBOSE)
            match = regex.match(line)
            if match is None:
                raise RuntimeError(line)

            person = self.population.person[int(match.group('initiator'))]
            if not person.hiv_positive:
                # If the initiator was not already HIV+, then make them such
                person.set_hiv_infection(self.timestep)

            transmission_coefficient = float(match.group('transmission_coeff'))
            person.set_transmission_coefficient(self.timestep,
                                                transmission_coefficient)

        elif line.startswith(' !A condom was '):
            pass
        elif line.find('circumcised') is not -1:
            pass
        elif line.find('Total FOI') is not -1:
            pass
        elif line.find('exposed but did not infect') is not -1:
            pass
        elif line.find(' infected ') is not -1:
            # Count how many people each HIV+ person infected.
            match = self.infected_regex.match(line)
            infector = self.population.person[int(match.group('infector'))]
            infectee = self.population.person[int(match.group('partner'))]
            infector.infected.append(infectee.person_id)
            infectee.infected_by = infector.person_id
            infectee.set_hiv_infection(self.timestep)

    def process_csw_retirement(self, line):
        """A CSW has retired.
        """
        match = self.csw_retirement_regex.match(line)
        pid = int(match.group('id'))
        self.population.person[pid].csw = False

    def timesteps(self, fptr):
        """Run through all timesteps in the simulation.
        """
        while True:
            line = next(fptr).rstrip()
            #print(line)
            if line.startswith('** Time '):
                #print(line)
                self.timestep = self.timestep + 1
                self.population.update_partnership_lists(self.timestep)
                #print(self.population.person[72410])
                #print(self.population.person[72410].traced)
                continue

            if self.timestep < self.trace_delay:
                # Don't bother processing anything until the trace delay
                # has been reached.
                continue

            if line == '':
                # skip blank lines
                pass
            elif line.startswith('Now creating initial partnerships...'):
                pass
            elif line.startswith('Tracing'):
                # Already handled this.  Skip past the next line which
                # identifies the individual.
                line = next(fptr).rstrip()
                self.population.add_to_population(line)
            elif line[0] == '#':
                self.process_acts(line)
            elif line[0] == '-':
                self.process_end_partnership(line)
            elif line[0] == '+':
                # partnership attempt
                pass
            elif line.startswith('  + '):
                self.new_partnership(line)
            elif line.startswith('  +x') or line.startswith('   x'):
                # Invalid partnership attempt.
                #
                # '+ Male 658 attempts to form 1 Casual partnerships:'
                # '  +x Male 658 attempted to draw from empty bucket'
                # '   x Partnership not formed!'   <== This one.
                #
                # Just skip these for now.
                pass
            elif self.sexual_debut_regex.match(line) is not None:
                pass
            elif self.transition_to_csw_regex.match(line) is not None:
                self.process_transition_to_csw(line)
            elif self.rerolls_risk_regex.match(line) is not None:
                self.reroll_risk(line)
            elif line.startswith('>> Today we mourn:'):
                line = next(fptr)
                self.process_death(line)
            elif self.csw_retirement_regex.match(line):
                self.process_csw_retirement(line)
            elif self.prevalent_hiv_regex.match(line):
                self.process_prevalent_hiv(line)
            elif self.incident_hiv_regex.match(line):
                self.process_incident_hiv(line)
            elif line.startswith(' !'):
                self.process_transmission_act(line)
            else:
                raise ParseRuntimeError(line)

    def _run(self):
        """Run through the trace file, processing all events."""
        with open(self.trace_file, 'r') as fptr:

            # Skip until timesteps start.
            #line = next(fptr)
            #while not line.startswith('** Time 1:'):
            #    line = next(fptr)

            #self.timestep = self.timestep + 1

            try:
                self.timesteps(fptr)
            except StopIteration:
                return


class ParseRuntimeError(RuntimeError):
    """Catch-all for unparseable lines."""
    def __init__(self, line):
        msg = "Unable to parse this line in the Singleperson txt file"
        msg += "'\n\n{0}'"
        msg = msg.format(line)
        RuntimeError.__init__(self, msg)
