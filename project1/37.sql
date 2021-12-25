-- Print the total level of Pokémon of type fire among the catched Pokémon.

SELECT  SUM(CP.level)
FROM    CatchedPokemon AS CP, Pokemon AS P
WHERE   CP.pid = P.id
    AND P.type = 'Fire';
