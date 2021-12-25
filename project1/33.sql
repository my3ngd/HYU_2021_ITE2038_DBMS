-- Print the names of the trainers who caught Psychic-type Pokémon in alphabetical order.

SELECT  T.name
FROM    Trainer AS T, CatchedPokemon AS CP, Pokemon AS P
WHERE   T.id = CP.owner_id
    AND CP.pid = P.id
    AND P.type = 'Psychic'
ORDER BY T.name ASC;
