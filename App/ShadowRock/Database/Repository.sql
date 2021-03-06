CREATE TABLE IF NOT EXISTS `Names` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`Category` TEXT NOT NULL,
	`Name`	TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS `Resources` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`TypeID`	INT NOT NULL,
	`Path`	TEXT,
	FOREIGN KEY(`TypeID`) REFERENCES `Names`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `ResourceDependencies` (
	`SourceID`	INT NOT NULL,
	`TargetID`	INT NOT NULL,
	FOREIGN KEY(`SourceID`) REFERENCES `Resources`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`TargetID`) REFERENCES `Resources`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `Entities` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`GroupID`	INT NOT NULL,
	`Name`	TEXT NOT NULL,
	FOREIGN KEY(`GroupID`) REFERENCES `EntityGroups`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `EntityGroups` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`Name`	TEXT NOT NULL
);
INSERT INTO `EntityGroups` VALUES (0, "Root");

CREATE TABLE IF NOT EXISTS `EntityComponents` (
	`ID`	INT NOT NULL,
	`ComponentID`	INT NOT NULL,
	FOREIGN KEY(`ID`) REFERENCES `Entities`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`ComponentID`) REFERENCES `Components`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `Components` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`Type`	INT NOT NULL,
	FOREIGN KEY(`Type`) REFERENCES `Names`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `ModelComponents` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`ResourceID` INT NOT NULL,
	`ViewDistance` FLOAT NOT NULL,
	FOREIGN KEY(`ID`) REFERENCES `Components`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`ResourceID`) REFERENCES `Resources`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `ModelComponentMaterials` (
	`ID`	INT NOT NULL,
	`MeshGroupID` INT NOT NULL,
	`ResourceID` INT NOT NULL,
	FOREIGN KEY(`ID`) REFERENCES `Components`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`ResourceID`) REFERENCES `Resources`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `CameraComponents` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`FieldOfView`	FLOAT NOT NULL,
	'Near'		FLOAT NOT NULL,
	'Far'		FLOAT NOT NULL,
	'Aspect'	FLOAT NOT NULL,
	FOREIGN KEY(`ID`) REFERENCES `Components`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `TransformComponents` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`X`		FLOAT NOT NULL,
	'Y'		FLOAT NOT NULL,
	'Z'		FLOAT NOT NULL,
	'Scale'	FLOAT NOT NULL,
	'Yaw'	FLOAT NOT NULL,
	'Pitch' FLOAT NOT NULL,
	'Roll'	FLOAT NOT NULL
);
CREATE TABLE IF NOT EXISTS `SpaceComponents` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`GroupID`	INT NOT NULL,
	FOREIGN KEY(`GroupID`) REFERENCES `EntityGroups`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE IF NOT EXISTS `EnvCubeComponents` (
	`ID`	INT NOT NULL PRIMARY KEY,
	`ResourceID`	INT NOT NULL,
	FOREIGN KEY(`ResourceID`) REFERENCES `Resources`(`ID`) ON DELETE CASCADE ON UPDATE CASCADE
);