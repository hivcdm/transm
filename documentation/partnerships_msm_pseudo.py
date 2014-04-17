population = # All people in simulation
partner_acquisition_rates = # See Transmission-Inputs, Partnerships sheet, B8
potential_partner_buckets = # See XML, partnershipTypes section, DmgProfile (e.g. SA:FEMALE:HETERO:SINGLE:NON_CSW)
potential_partner_bucket_weights = # ^^
assortativity = # Chance of selecting only in own risk group when forming partnership
serosorting = # Chance of selecting only in own serostatus group when forming partnership
seropositioning = # Chance of selecting only opposite "role" when forming partnership
entities = # Same as population but stratified by demographic profile (sexaul activity, gender, sexual orientation, relationship status, employment)

# Return random person from given age range, partnership type, and risk group
# Expands age range outwards by six months until someone is found or groups are exhausted
def get_random_person(desire_age_min, desired_age_max, partnership_type, risk_group, hiv_status, role):
	# High assortativity means partner's risk should match initiators risk
	# Low means partner's risk is ignored
	if random.random(0, 1) < assortativity:
		partner_risk = risk_group
	else:
		partner_risk = ANY

	# High serosorting means partner's hiv status must match that of the initiator
	if random.random(0, 1) < serosorting:
		partner_status = hiv_status
	else:
		partner_status = ANY

	# High seropositioning means partner's hiv status must match that of the initiator
	if random.random(0, 1) < seropositioning:
		# Opposite of insertive is receptive and either
		# Opposite of receptive is insertive and either
		# Opposite of either is ANY
		partner_role = role.opposite
	else:
		partner_role = ANY

	# Marbles represent the number of people in the given age range and matching risk, serostatus, and role
	marbles = sum(num_in_group(age_buckets[desired_age], partner_risk, partner_status, partner_role))

	# Found no one matching, expand the search
	if marbles == 0:
		# Make sure that there is *someone* of any age with the desired risk group
		marbles = sum(num_in_group(age_buckets, partner_risk, partner_status, partner_role))

		if marbles > 0: # Someone was found, expand age range
			return get_random_person(desired_age_min - 6, desired_age_max + 6, partnership_type, partner_risk, partner_status, partner_role)
		else: # No one found
			return None

	# We want to pick a random "marble" from all those found
	random_pick = random.random(0, marbles)

	for age_bucket in age_buckets:
		# Our random marble is in this age range
		if random_pick < num_in_group(age_buckets, partner_risk, partner_status, partner_role):
			# Select someone at random, not necessarily based on "random_pick"
			partner = random_person_in_age_bucket(age_bucket, partner_risk, partner_status, partner_role)
			# We don't want this person to be chosen again for now
			entities.remove_entity(partner)

			return partner
		else: # Decrement random_pick by the number of people with the chosen risk group
			random_pick = (random_pick - num_in_group(age_buckets, partner_risk, partner_status, partner_role))

	# This shouldn't happen, we already know how many marbles there are
	return None

# After expired partnerships are dissolved, form new partnerships
def create_partnerships():
	for initiator in population.msms:
		for partnership_type in ['steady', 'regular', 'casual']:
			partner_acquisition_rate = partner_acquisition_rates[partnership_type][initiator.risk_group]
			num_partners = random.poission(partner_acquisition_rate)

			while num_partners > 0:
				# Selected randomly based on weights (i.e. <weightedValue>1</weightedValue> in XML)
				partner_bucket = random.weighted(potential_partner_buckets, potential_partner_bucket_weights)
				
				age_difference_years = random.normal(average_years_younger)
				age_difference_months = age_difference_years * 12
				# Six months on either side
				desired_age_min = (initiator.age - age_difference_months) - 6
				desired_age_max = (initiator.age - age_difference_months) + 6

				# Try ten times and then give up
				for i in [0..10]:
					partner = get_random_person(desired_age_min, desired_age_max, partnership_type, initiator.risk_group, initiator.hiv_status, initiator.role)

					if partner != None:
						if initiator.is_partnered_with(partner):
							# Make a note of this, but don't reform
							entities.add_entity(partner)
						else:
							break # break out of "for i in [0..10]"

				if partner != None:
					create_partnership(initiator, partner, partnership_type)

				num_partners = num_partners - 1