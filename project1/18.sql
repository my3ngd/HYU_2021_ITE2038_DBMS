-- Print the names of Grass-type Pokémon in alphabetical order

SELECT  P.name
FROM    Pokemon AS P
WHERE   P.type = 'Grass'
ORDER BY P.name ASC;
