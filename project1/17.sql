-- Print the average level of all water-type Pokémon caught by the trainer.

SELECT  AVG(CP.level)
FROM    CatchedPokemon AS CP, Pokemon AS P
WHERE   CP.pid = P.id
    AND P.type = 'water';
