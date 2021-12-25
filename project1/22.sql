-- Print the IDs and names of all Pokémon in the Pokémon Encyclopedia in ascending order of ID.

SELECT  P.id, P.name
FROM    Pokemon AS P
ORDER BY P.id ASC;
