#stores data for an output file
class StatFile:
    def __init__(self, filepath, hrow):
        with open(filepath) as fOutput:
            allRows = [line.strip() for line in fOutput.readlines()]
            self.headers = allRows[hrow].split("\t")
            self.data = {}
            monthCol = self.headers.index("Month")
            for dRow in allRows[hrow+1:]:
                dataRow = dRow.split("\t")
                try:
                    if dataRow[monthCol].lower() == "init":
                        month = 0
                    else:
                        month = int(dataRow[monthCol])
                    
                except ValueError:
                    break
                else:
                    self.data[month] = dataRow
    def get_data(self, header, month, n = 1):
        index = -1
        for j in range(n):
            index = self.headers.index(header, index+1)
        return self.data[month][index]
    def get_all(self, header, n = 1):
        month = 0
        dataToReturn = []
        index = -1
        for j in range(n):
            index = self.headers.index(header, index+1)

        try:
            while True:
                dataToReturn.append(self.data[month][index])
                month+=1
        except KeyError:
            return dataToReturn
